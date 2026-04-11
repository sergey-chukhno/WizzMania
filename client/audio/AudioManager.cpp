#include "AudioManager.h"
#include <QDateTime>
#include <QDebug>
#include <QtEndian>

struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t overallSize; // File size - 8
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtLength = 16;
  uint16_t audioFormat = 1; // PCM
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize;
};

AudioManager::AudioManager(QObject *parent) : QObject(parent) { setupFormat(); }

AudioManager::~AudioManager() {
  if (m_audioSource) {
    m_audioSource->stop();
  }
  if (m_audioSink) {
    m_audioSink->stop();
  }
}

void AudioManager::setupFormat() {
  m_format.setSampleRate(16000);
  m_format.setChannelCount(1);
  m_format.setSampleFormat(QAudioFormat::Int16);
}

bool AudioManager::startRecording() {
  const auto inputs = QMediaDevices::audioInputs();
  if (inputs.isEmpty()) {
    qWarning() << "No audio input device found!";
    return false;
  }

  auto device = QMediaDevices::defaultAudioInput();
  m_format = device.preferredFormat();
  m_format.setSampleFormat(QAudioFormat::Int16);

  if (m_audioSource) {
    m_audioSource.reset();
  }
  m_audioSource = std::make_unique<QAudioSource>(device, m_format);
  
  m_inputBuffer.buffer().clear();
  m_inputBuffer.open(QIODevice::WriteOnly | QIODevice::Truncate);

  m_audioSource->setVolume(1.0f);
  m_audioSource->start(&m_inputBuffer);
  m_isRecording = true;
  m_recordingStartTime = QDateTime::currentMSecsSinceEpoch();

  return true;
}

std::vector<uint8_t> AudioManager::stopRecording(uint16_t &duration) {
  if (!m_isRecording || !m_audioSource) {
    return {};
  }

  m_audioSource->stop();
  m_inputBuffer.close();
  m_isRecording = false;

  qint64 endTime = QDateTime::currentMSecsSinceEpoch();
  duration = static_cast<uint16_t>((endTime - m_recordingStartTime) / 1000);
  if (duration == 0)
    duration = 1; 

  QByteArray rawPcm = m_inputBuffer.buffer();
  QByteArray wavData = addWavHeader(rawPcm);

  return std::vector<uint8_t>(wavData.begin(), wavData.end());
}

QByteArray AudioManager::addWavHeader(const QByteArray &rawPcmData) {
  WavHeader header;
  header.numChannels = static_cast<uint16_t>(m_format.channelCount());
  header.sampleRate = static_cast<uint32_t>(m_format.sampleRate());
  header.bitsPerSample = 16; 

  header.byteRate =
      header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
  header.dataSize = static_cast<uint32_t>(rawPcmData.size());
  header.overallSize = header.dataSize + 36;

  QByteArray wavFile;
  wavFile.resize(sizeof(WavHeader) + rawPcmData.size());

  std::memcpy(wavFile.data(), &header, sizeof(WavHeader));
  std::memcpy(wavFile.data() + sizeof(WavHeader), rawPcmData.data(),
              rawPcmData.size());

  return wavFile;
}

void AudioManager::playAudio(const std::vector<uint8_t> &wavData) {
  if (wavData.size() < sizeof(WavHeader)) {
    qWarning() << "Invalid WAV data size";
    return;
  }

  auto device = QMediaDevices::defaultAudioOutput();
  if (device.isNull()) {
    qWarning() << "No audio output device found!";
    return;
  }

  if (m_audioSink) {
    m_audioSink->stop();
    m_audioSink.reset();
  }

  const WavHeader *header = reinterpret_cast<const WavHeader *>(wavData.data());

  QAudioFormat playFormat;
  playFormat.setSampleRate(header->sampleRate);
  playFormat.setChannelCount(header->numChannels);
  playFormat.setSampleFormat(QAudioFormat::Int16); 

  m_audioSink = std::make_unique<QAudioSink>(device, playFormat);
  m_audioSink->setVolume(1.0f); 

  connect(m_audioSink.get(), &QAudioSink::stateChanged, this,
          [this](QAudio::State state) {
            if (state == QAudio::ActiveState) {
              emit playbackStarted();
            } else if (state == QAudio::IdleState ||
                       state == QAudio::StoppedState) {
              emit playbackStopped();
            }
          });

  size_t headerSize = sizeof(WavHeader);
  if (wavData.size() > headerSize) {
    QByteArray pcmData(
        reinterpret_cast<const char *>(wavData.data() + headerSize),
        wavData.size() - headerSize);

    m_outputBuffer.close();
    m_outputBuffer.setData(pcmData);
    m_outputBuffer.open(QIODevice::ReadOnly);

    m_audioSink->start(&m_outputBuffer);
  }
}
