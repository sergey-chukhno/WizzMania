#pragma once

#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QBuffer>
#include <QByteArray>
#include <QMediaDevices>
#include <QObject>
#include <vector>

class AudioManager : public QObject {
  Q_OBJECT

public:
  explicit AudioManager(QObject *parent = nullptr);
  ~AudioManager();

  // Recording
  bool startRecording();
  std::vector<uint8_t> stopRecording(uint16_t &duration); // Returns WAV data

  // Playback
  void playAudio(const std::vector<uint8_t> &wavData);

  bool isRecording() const { return m_isRecording; }

signals:
  void playbackStarted();
  void playbackStopped();

private:
  void setupFormat();
  QByteArray addWavHeader(const QByteArray &rawPcmData);

  QAudioFormat m_format;
  QAudioSource *m_audioSource = nullptr;
  QAudioSink *m_audioSink = nullptr;

  QBuffer m_inputBuffer;  // For recording
  QBuffer m_outputBuffer; // For playback (sink reads from here)

  bool m_isRecording = false;
  qint64 m_recordingStartTime = 0;
};
