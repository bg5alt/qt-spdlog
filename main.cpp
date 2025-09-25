
#include <QApplication>
#include <QFontDatabase>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <QFile>

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <thread>
#include <QThread>
#include <QtConcurrent>

#include "logmanager.h"

#include <sstream>
#include <string>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#endif



int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // qApp->setFont(QFont("Courier New", 13));


    LogInit();
    LogAddConfig({"log","logs","dzh.log",0,1024*5,10,false});
    LogAddConfig({"bg","logs","bg.log",0,1024*5,10,false});

    // 获取日志器并记录日志
    LogTrace() << "This is an trace message.";
    LogInfo() << "This is an info message.";
    LogDebug() << "This is a debug message.";
    LogWarn() << "This is an warn message.";
    LogError() << "This is an error message.";
    LogCritical() << "This is an critical message.";

    LogTrace("bg") << "This is an trace message.";
    LogInfo("bg") << "This is an info message.";
    LogDebug("bg") << "This is a debug message.";
    LogWarn("bg") << "This is an warn message.";
    LogError("bg") << "This is an error message.";
    LogCritical("bg") << "This is an critical message.";


    // 设置日志级别
    // dLogSetLevel(1);
    LogDebug() << "This is a debug message.";

    int i = 10;
    double d = 1.123;
    float f = 1.234f;
    std::string s = " std::string ";
    qint8     q8 = 8;
    qint16    q16 = 16;
    qint32    q32 = 32;
    qint64    q64 = 64;
    int8_t    i8 = 8;
    int16_t   i16 = 16;
    int32_t   i32 = 32;
    int64_t   i64 = 64;
    uint8_t   u8 = 8;
    uint16_t  u16 = 16;
    uint32_t  u32 = 32;
    uint64_t  u64 = 64;



    QString str = "Hello, World!";
    QStringList strList = {"a","b","c"};
    QList<QString> qListStr = {"a","b","c"};
    QVector<double> vec = {4,5,6};
    QVector<QString> vecs = {"a","b","c"};
    QMap<QString, QString> _maps = {{"k1","v1"},{"k2","v2"},{"k3","v3"}};
    QMap<QString, int> _mapi = {{"k1",1},{"k2",2},{"k3",3}};
    QHash<QString, QString> _hash = {{"k1","v1"},{"k2","v2"},{"k3","v3"}};
    QVariant var = {"abc"};
    QVariantList vars = {"abc",123,4.56};
    QVariantMap var_mpa = {{"k1","v1"},{"k2",123},{"k3","v3"}};

    QVariantMap vmaps;
    vmaps["k1"] = "v1";

    vmaps["k2"] = vars;
    vmaps["k3"] = var_mpa;


    LogInfo() << "-------支持类型测试------";
    LogInfo() << " q8 " << static_cast<short>(q8) << " q16 " << q16 << " q32 " << q32 << " q64 " << q64;
    LogInfo() << " i8 " << static_cast<short>(i8) << " i16 " << i16 << " i32 " << i32 << " i64 " << i64;
    LogInfo() << " u8 " << static_cast<unsigned short>(u8) << " u16 " << u16 << " u32 " << u32 << " u64 " << u64;
    LogInfo() << " i " << i << " d " << d << " f " << f << " s " << s;

    LogInfo() << " str " << str;
    LogInfo() << " strList " << strList;
    LogInfo() << " qListStr " << qListStr;
    LogInfo() << " vec " << vec;
    LogInfo() << " vecs " << vecs;
    LogInfo() << " _maps " << _maps;
    LogInfo() << " _mapi " << _mapi;
    LogInfo() << " _hash " << _hash;
    LogInfo() << " var " << var;
    LogInfo() << " vars " << vars;
    LogInfo() << " var_mpa " << var_mpa;
    LogInfo() << " vmaps " << vmaps;
    // dSetLevel(4);

    LogInfo() << " ----------支持结构体序列化测试--------- ";
    LogInfo() << QString(" ----------支持结构体序列化测试--------- ");
    // 获取当前线程 ID
    std::thread::id thread_id = std::this_thread::get_id();

    // 将线程 ID 转换为字符串
    std::stringstream ss;
    ss << thread_id;

    LogInfo() << ss.str();
    LogInfo("bg") <<"pid:"<< getpid() <<" thread:"<< ss.str();
    QtConcurrent::run([]()
    {
        LogInfo() << " ------------------- ";
        // 获取当前线程 ID
        std::thread::id thread_id = std::this_thread::get_id();

        // 将线程 ID 转换为字符串
        std::stringstream ss;
        ss << thread_id;
        LogInfo() << ss.str();
        LogInfo("bg") <<"pid:"<< getpid() <<" thread:"<< ss.str();
    });
    LogInfo() << QColor("#FFFFFF");
    LogInfo() << QColor(0xFFFFFF);
    LogInfo() << QSize(32,32);
    LogInfo() << QSizeF(32.0,32.2);
    LogInfo() << QPoint(32,32);
    LogInfo() << QPointF(32.0,32.1);
    LogInfo() << QRect(32,32,32,32);
    LogInfo() << QRectF(32,32,32,32);


    QByteArray byte = " QByteArray ";

    LogInfo() << byte;

    QQueue<int> que;
    que.append(1);
    que.append(2);
    que.append(3);
    que.append(4);

    LogInfo() << que;

    std::queue<int> qu;
    qu.push(1);
    qu.push(2);
    qu.push(3);
    qu.push(4);

    LogInfo() << qu;



    //return 0;
    return QApplication::exec();
}
