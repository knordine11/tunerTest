#include "widget.h"
#include "ui_widget.h"
#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>
#include <QtEndian>
#include "fftstuff.h"
#include <qtimer.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <QMessageBox>

int curLessonInt = 0;
int orientation [21] = {1,2,3,4,5,6,7,8,1,8,1,8,1,8,7,6,5,4,3,2,1};
extern bool collectMicData;
extern double rec_arr[];    // DEFINED AS DOUBLE FOR FFTW
extern int rec_arr_cnt;
extern int frame_start;
extern int frame_size;
extern int frame_end;
extern int rec_arr_end;
QByteArray bufferReadTo;
QByteArray *currentAudioOut;
QList<QByteArray> note_tone;
int major_number_list[] = {0,2,4,5,7,9,11,12};
int tonic_nunber;
int audio_number_list;
const QList <QString> note_letters = {"C", "C#", "D", "D#", "E",
                                     "F", "F#", "G", "G#", "A", "A#", "B" };
extern float note_acc;
extern int noteC_no;
extern int noteC_oct;
extern float nn_l[12];
extern QList<QByteArray> note_tone;
FftStuff ftw;
int accValue = 0;


Microphone::Microphone(const QAudioFormat &format) : m_format(format) {
    qDebug()<<" YOU SHOULD SEE THIS ";
}

Microphone::~Microphone()
{

}

void Microphone::start()
{
    open(QIODevice::WriteOnly);
}

void Microphone::stop()
{
    close();
}

qint64 Microphone::readData(char * /* data */, qint64 /* maxlen */)      // NOT USED IN EXAMPLE
{
    qDebug()<<" qint64 Microphone::readData(char * /* data */, qint64 /* maxlen */)  ";
    return 0;
}

qreal Microphone::getNoteValue(const char *data, qint64 len) const
{
    const int channelBytes = m_format.bytesPerSample();
    const int sampleBytes = m_format.bytesPerFrame();
    const int numSamples = len / sampleBytes;

    float maxValue = 0;
    auto *ptr = reinterpret_cast<const unsigned char *>(data);

    for (int i = 0; i < numSamples; ++i) {
        float value = m_format.normalizedSampleValue(ptr);
        rec_arr[rec_arr_cnt]=value;
        //maxValue = qMax(value, maxValue);
        ptr += channelBytes;
        rec_arr_cnt++;
    };
    qDebug()<<"    END OF DATA FROM MIC---------->   rec_arr_cnt "<<rec_arr_cnt;
    qDebug() <<"frame start and end values: " << frame_start << " " << frame_end;
    if(rec_arr_cnt > frame_end){
        cout<<"\n  NEXT FRAME $$$$ "<<frame_start<<endl;

        ftw.DoIt(frame_start, frame_size);
        frame_start = frame_end;
        frame_end = frame_end + frame_size;}
    qDebug()<< "current thread: " << QThread::currentThread();
    if (rec_arr_cnt > 200000)
    {
        qDebug() <<"                      restart here";
        rec_arr_cnt = 0;

        emit on_TimeOut();
        qDebug() <<"                      post restart here";
        return 0;
    }
    return maxValue;
}

qint64 Microphone::writeData(const char *data, qint64 len)
{
    qDebug() << "enter writeData" << rec_arr_cnt;
    m_level = getNoteValue(data, len);
    return len;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Tuner test");
    initializeWindow();
    initializeAudio(QMediaDevices::defaultAudioInput());
    FftStuff fts;
    connect(&ftw, &FftStuff::valueChanged,this, &Widget::updateKBnote, Qt::QueuedConnection);
    connect(m_Microphone, &Microphone::on_TimeOut, this, &Widget::TimeOut);
}

Widget::~Widget()
{
    MicThread.terminate();
    delete m_audioSource;
    delete m_Microphone;
    delete ui;
}

void Widget::paintEvent(QPaintEvent * event)
{
    QPixmap pix(100,60);
    pix.fill(Qt::white);
    //create a QPainter and pass a pointer to the device.
    QPainter *painter = new QPainter(&pix);
    QPen outsidePen(Qt::red, 4, Qt::SolidLine);
    painter->setPen(outsidePen);
    painter->drawEllipse(35, 15, 30, 30);
    QPen insidePen(Qt::green, 4, Qt::SolidLine);
    painter->setPen(insidePen);
    painter->drawEllipse(40 + accValue, 20, 20, 20);
    QPen linePen(Qt::black, 1, Qt::SolidLine);
    painter->setPen(linePen);
    painter->drawLine(0, 30, 100, 30);
    painter->drawLine(50, 0, 50, 60);
    painter->end();
    ui->lbTuner->setPixmap(pix);
}

void Widget::on_btnStart_clicked()
{
    m_Microphone->moveToThread(&MicThread);
    MicThread.setObjectName("MicThread");
    MicThread.start();

    restartAudioStream();
    qDebug() << "start pushed...";
    ui->btnStart->setVisible(false);
}

void Widget::updateKBnote(int kbValue, float acc)
{

    int letter = kbValue%12;
    int octaveValue = kbValue/12;
    QString theNote = note_letters[letter] + QString::number(octaveValue);
    qDebug() << "knNote... " << theNote;
    QString alphaNote = note_letters[letter] + QString::number(octaveValue);
    qDebug() << "alphaNote... " << alphaNote;
    ui->lbNote->setText(alphaNote);
    qDebug() << "acc is: " << (int)(acc * 1000);
    accValue = (int)(acc * 1000);
    ui->lbTuner->repaint();
}

void Widget::stop_mic()
{
    m_audioSource->reset();
    m_audioSource->start();
}

void Widget::TimeOut()
{
    MicThread.terminate();
    rec_arr_cnt = 0;
    m_Microphone->reset();
    m_audioSource->reset();
    frame_start = 0;
    frame_end = 2048;
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Timed Out", "Continue?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "continuing...";

        m_Microphone->moveToThread(&MicThread);
        MicThread.setObjectName("MicThread");
        MicThread.start();
        m_audioSource->start(m_Microphone);
    } else {
        qDebug() << "No was clicked";
        QApplication::quit();
    }
}

void Widget::initializeWindow()
{
    const QAudioDevice &defaultDeviceInfo = QMediaDevices::defaultAudioInput();
}

void Widget::initializeAudio(const QAudioDevice &deviceInfo)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_Microphone = (new Microphone(format));
    m_audioSource = (new QAudioSource(deviceInfo, format));
    qDebug() <<  "buffer size: " << m_audioSource->bufferSize();
    m_Microphone->start();

}

void Widget::restartAudioStream()
{
    m_audioSource->stop();
    qDebug()<< "is main: " << QThread::isMainThread();
    m_audioSource->start(m_Microphone);
    qDebug() << "============================";
}

void Widget::on_btnStop_clicked()
{
    m_audioSource->stop();
}
