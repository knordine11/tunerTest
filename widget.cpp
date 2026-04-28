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
    // qDebug()<<"    END OF DATA FROM MIC---------->   rec_arr_cnt "<<rec_arr_cnt;

    if(rec_arr_cnt > frame_end){
        cout<<"\n  NEXT FRAME $$$$ "<<frame_start<<endl;

        ftw.DoIt(frame_start, frame_size);
        frame_start = frame_end;
        frame_end = frame_end + frame_size;}

    // emit void haltstream();
    if (rec_arr_cnt > 200000)
    {
        //emit on_stopMic();
        // qDebug() <<"                      restart here";
        // qDebug() <<"                   Microphone::pos()  "<<Microphone::pos();
    }
    return maxValue;
}

qint64 Microphone::writeData(const char *data, qint64 len)
{
    if(collectMicData)
    {
        // qDebug() <<" %%%%%% :writeData(const char *data, qint64 len)  "<<&data<<" len "<<len;
        m_level = getNoteValue(data, len);
    }
    return len;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Tuner test");
    initializeAudio(QMediaDevices::defaultAudioInput());

}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *event)
{
    QPixmap pix(100,60);
    pix.fill(Qt::white);
    //create a QPainter and pass a pointer to the device.
    //A paint device can be a QWidget, a QPixmap or a QImage
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
    initializeWindow();
    initializeAudio(QMediaDevices::defaultAudioInput());
    FftStuff fts;
    connect(&ftw, &FftStuff::valueChanged,this, &Widget::updateKBnote, Qt::QueuedConnection);
    // connect(&mic,&Microphone::on_stopMic, this, &Widget::stop_mic);
    restartAudioStream();
    qDebug() << "end start...";
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
    m_audioSource->stop();
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

    m_Microphone.reset(new Microphone(format));
    m_audioSource.reset(new QAudioSource(deviceInfo, format));
    m_Microphone->start();

}

void Widget::restartAudioStream()
{
    m_audioSource->stop();
    qDebug()<<" restartAudioStream() |  m_pullMode  "<<m_pullMode;
    if (m_pullMode) {
        // pull mode: QAudioSource provides a QIODevice to pull from
        auto *io = m_audioSource->start();
        if (!io)
            return;

        connect(io, &QIODevice::readyRead, [this, io]() {
            static const qint64 BufferSize = 4096;
            const qint64 len = qMin(m_audioSource->bytesAvailable(), BufferSize);

            QByteArray buffer(len, 0);
            qint64 l = io->read(buffer.data(), len);

            qDebug() << "io->read(buffer.data(), len) "<<l;

            if (l > 0) {
                const qreal level = m_Microphone->getNoteValue(buffer.constData(), l);
                // qDebug()<<" level  " << level;
            }
        });
    } else {
        // push mode: QIODevice pushes data into QIODevice
        m_audioSource->start(m_Microphone.get());
        qDebug() << "============================";
    }
}

void Widget::on_btnStop_clicked()
{
    emit on_click_stopMic();
    m_audioSource->stop();
}

