#ifndef ENHANCED_STREAM_H
#define ENHANCED_STREAM_H
#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <type_traits>

// Qt 基本类型
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QQueue>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
/// qt 几何类型
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>
/// qt 颜色类型
#include <QColor>
/// qt 字符串类型
#include <QString>
#include <QChar>


/**
 * @brief 增强字符串流，使用运算符重载实现流式API
 *
 * 支持基本数据类型、容器以及自定义类型的字符串转换
 * 使用示例：
 *   std::string s = EStream() << 42 << "hello" << std::vector{1,2,3};
 */

class EStream {

    std::ostringstream oss;
    int _depth = 0; /// 当前递归深度
    static constexpr int _max_depth = 100;
public:
    // 基本数据类型转换
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
    EStream& operator<<(T value) {
        oss << value;
        return *this;
    }
    // 字符串类型转换
    EStream& operator<<(const char* value) {
        oss << value;
        return *this;
    }

    EStream& operator<<(const std::string& value) {
        oss << value;
        return *this;
    }

    // 容器类型转换
    template <typename T>
    EStream& operator<<(const std::vector<T>& vec) {
        return writeContainer(vec, '[', ']');
    }

    template <typename T>
    EStream& operator<<(const std::list<T>& lst) {
        return writeContainer(lst, '[', ']');
    }

    template <typename T>
    EStream& operator<<(const std::set<T>& set) {
        return writeContainer(set, '[', ']');
    }
    template <typename T>
    EStream& operator<<(const std::queue<T>& que) {
        auto tmp = que;

        oss << '{';
        bool first = true;
        while (!tmp.empty()) {
            if (!first) oss << ", ";
            first = false;
            *this << tmp.front();
            tmp.pop();
        }
        oss << '}';
        return *this;

    }


    template <typename K, typename V>
    EStream& operator<<(const std::map<K, V>& kv) {
        return writeStdKeyValue(kv,'{','}');
    }

    // 智能指针类型转换
    template <typename T>
    EStream& operator<<(const std::shared_ptr<T>& ptr) {
        if (ptr) {
            *this << *ptr;
        } else {
            oss << "nullptr";
        }
        return *this;
    }

    template <typename T>
    EStream& operator<<(const std::unique_ptr<T>& ptr) {
        if (ptr) {
            *this << *ptr;
        } else {
            oss << "nullptr";
        }
        return *this;
    }

    template <typename T>
    EStream& operator<<(const std::weak_ptr<T>& ptr) {
        if (auto locked = ptr.lock()) {
            *this << *locked;
        } else {
            oss << "expired weak_ptr";
        }
        return *this;
    }

    // Optional类型转换
    template <typename T>
    EStream& operator<<(const std::optional<T>& opt) {
        if (opt.has_value()) {
            *this << opt.value();
        } else {
            oss << "null opt";
        }
        return *this;
    }

    // 字符串类型
    EStream& operator<<(const QString& value) {
        oss << value.toUtf8().constData();
        return *this;
    }

    EStream& operator<<(const QByteArray& value) {
        oss << value.constData();
        return *this;
    }
    EStream& operator<<(const QStringList& lst)
    {
        return writeContainer(lst,'[',']');
    }
    EStream& operator<<(const QPoint& value) {
        oss << "{"<<value.x()<<","<<value.y()<<"}";
        return *this;
    }
    EStream& operator<<(const QPointF& value) {
        oss << "{"<<value.x()<<","<<value.y()<<"}";
        return *this;
    }
    EStream& operator<<(const QSize& value) {
        oss << "{"<<value.width()<<","<<value.height()<<"}";
        return *this;
    }
    EStream& operator<<(const QSizeF& value) {
        oss << "{"<<value.width()<<","<<value.height()<<"}";
        return *this;
    }
    EStream& operator<<(const QRect& value) {
        oss << "{"<<value.x()<<","<<value.y()<<","<<value.width()<<","<<value.height()<<"}";
        return *this;
    }
    EStream& operator<<(const QRectF& value) {
        oss << "{"<<value.x()<<","<<value.y()<<","<<value.width()<<","<<value.height()<<"}";
        return *this;
    }
    EStream& operator<<(const QColor& value) {
        oss << value.name().toUtf8().constData();
        return *this;
    }
    EStream& operator<<(const QChar& value) {
        oss << value.toLatin1();
        return *this;
    }


    // Qt 容器处理
    // // 通用容器处理 (QList/QVector/QStringList)
    template <typename T>
    EStream& operator<<(const QList<T>& lst)
    {
        return writeContainer(lst,'[',']');
    }

    template <typename T>
    EStream& operator<<(const QVector<T>& lst)
    {
        return writeContainer(lst,'[',']');
    }
    template <typename T>
    EStream& operator<<(const QQueue<T>& que) {
        return writeContainer(que, '{', '}');
    }

    // QMap 处理
    template <typename K, typename V>
    EStream& operator<<(const QMap<K, V>& kv) {
        return writeKeyValue(kv,'{','}');
    }

    // QHash 处理
    template <typename K, typename V>
    EStream& operator<<(const QHash<K, V>& kv) {
        return writeKeyValue(kv,'{','}');
    }

//--------------------------------------------------
// QVariant 系列处理
//--------------------------------------------------
    EStream& operator<<(const QVariant& var) {
        switch (var.type()) {
            case QVariant::Int:
                oss << var.toInt();
                break;
            case QVariant::UInt:
                oss << var.toUInt();
                break;
            case QVariant::LongLong:
                oss << var.toLongLong();
                break;
            case QVariant::ULongLong:
                oss << var.toULongLong();
                break;
            case QVariant::Double:
                oss << var.toDouble();
                break;
            case QVariant::Bool:
                oss << var.toBool();
                break;
            case QVariant::String:
                *this << var.toString();
                break;
            case QVariant::ByteArray:
                *this << var.toByteArray();
                break;
            case QVariant::List:
                *this << var.toList();
                break;
            case QVariant::Map:
                *this << var.toMap();
                break;
            case QVariant::Color:
                *this << var.value<QColor>();
                break;
            case QVariant::Point:
                *this << var.toPoint();
                break;
            case QVariant::PointF:
                *this << var.toPointF();
                break;
            case QVariant::Size:
                *this << var.toSize();
                break;
            case QVariant::SizeF:
                *this << var.toSizeF();
                break;
            case QVariant::Rect:
                *this << var.toRect();
                break;
            case QVariant::RectF:
                *this << var.toRectF();
                break;
            default:
                oss << var.typeName();
                break;
        }
        return *this;
    }

    EStream& operator<<(const QVariantList& lst) {
        return writeContainer(lst,'[',']');
    }

    EStream& operator<<(const QVariantMap& kv) {
        return writeKeyValue(kv,'{','}');
    }

//--------------------------------------------------
    // 转换为字符串
    explicit operator std::string() const {
        return oss.str();
    }

    // 获取内部流
    const std::ostringstream& stream() const {
        return oss;
    }


private:
    // 辅助函数：写入容器
    template <typename Container>
    EStream& writeContainer(const Container& container, char open='[', char close=']') {
        if (_depth > _max_depth) {
            oss << " limit max depth";
            return *this;
        }
        _depth++;
        oss << open;
        bool first = true;
        for (auto it = container.begin(); it != container.end(); ++it) {
            if (!first) oss << ", ";
            first = false;
            *this << *it;
        }
        oss << close;
        _depth--;
        return *this;
    }
    // 辅助函数：写入容器
    template <typename Container>
    EStream& writeKeyValue(const Container& container, char open='{', char close='}') {
        if (_depth > _max_depth) {
            oss << " limit max depth";
            return *this;
        }
        _depth++;
        oss << open;
        bool first = true;
        for (auto it = container.begin(); it != container.end(); ++it) {
            if (!first) oss << ", ";
            first = false;
            *this << it.key() << ": " << it.value();
        }
        oss << close;
        _depth--;
        return *this;
    }
    // 辅助函数：写入容器
    template <typename Container>
    EStream& writeStdKeyValue(const Container& container, char open='{', char close='}') {
        oss << open;
        bool first = true;
        for (const auto& [key, value] : container) {
            if (!first) oss << ", ";
            first = false;
            *this << key << ": " << value;
        }
        oss << close;
        return *this;
    }
};
//--------------------------------------------------
// Qt 基础类型处理
//--------------------------------------------------

// inline EStream& operator<<(EStream& stream, int8_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, int16_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, int32_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, int64_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, uint8_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, uint16_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, uint32_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
// inline EStream& operator<<(EStream& stream, uint64_t value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
//
// inline EStream& operator<<(EStream& stream, double value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
//
// inline EStream& operator<<(EStream& stream, float value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
//
// inline EStream& operator<<(EStream& stream, const std::string& value) {
//     if (stream.logger_ && stream.logger_->should_log(stream.level_)) {
//         stream.ss_ << value;
//     }
//     return stream;
// }
//
// // 固定宽度整数类型
// inline EStream& operator<<(EStream& stream, const char* value) {
//     stream.ss_ << value;
//     return stream;
// }
//

#endif // ENHANCED_STREAM_H
