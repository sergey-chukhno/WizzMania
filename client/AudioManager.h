#pragma once

#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QBuffer>
#include <QByteArray>
#include <QMediaDevices>
#include <QObject>
#include <memory>
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
  std::unique_ptr<QAudioSource> m_audioSource;
  std::unique_ptr<QAudioSink> m_audioSink;

  QBuffer m_inputBuffer;  // For recording
  QBuffer m_outputBuffer; // For playback (sink reads from here)

  bool m_isRecording = false;
  qint64 m_recordingStartTime = 0;
};
