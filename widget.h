#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaDevices>
#include <QAudioSource>
#include <QMediaDevices>
#include <QPixmap>
#include <QPainter>
#include <QThread>

extern double rec_arr[];
extern int rec_arr_cnt;
extern int arr_size;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Microphone : public QIODevice
{
    Q_OBJECT

public:
    Microphone(const QAudioFormat &format);
    ~Microphone();

    void start();
    void stop();

    qreal level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

    qreal getNoteValue(const char *data, qint64 len) const;

signals:
    void levelChanged(qreal level);
    void on_TimeOut() const;

private:
    const QAudioFormat m_format;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    QThread MicThread;

protected:
    void paintEvent(QPaintEvent *event);

public slots:    
    void updateKBnote(int kbValue, float acc);
    void stop_mic();
    void TimeOut();

private slots:
    void on_btnStart_clicked();
    void on_btnStop_clicked();

private:
    void initializeWindow();
    void initializeAudio(const QAudioDevice &deviceInfo);
    void restartAudioStream();

private:
    // Owned by layout
    QMediaDevices *m_devices = nullptr;
    Microphone *m_Microphone;
    QAudioSource *m_audioSource;
    bool m_pullMode = false;
private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
