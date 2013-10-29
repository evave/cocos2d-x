#include "platform/CCFileUtils.h"
#include "CCPlatformMacros.h"
#include "platform/CCImageCommon_cpp.h"
#include "platform/CCImage.h"

#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QString>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsSimpleTextItem>
#include <QStyleOptionGraphicsItem>
#include <QFontDatabase>
#include <QSet>

namespace cocos2d {

// as FcFontMatch is quite an expensive call, cache the results of getFontFile
static std::map<std::string, std::string> fontCache;

namespace {
class FontUtilsQt
{
public:
    static int fontFlags(Image::TextAlign align)
    {
        int metricsFlags = 0;
        switch (align) {
        case Image::TextAlign::CENTER:
            metricsFlags = Qt::AlignCenter;
            break;
        case Image::TextAlign::TOP:
            metricsFlags = Qt::AlignHCenter | Qt::AlignTop;
            break;
        case Image::TextAlign::TOP_RIGHT:
            metricsFlags |= Qt::AlignRight | Qt::AlignTop;
            break;
        case Image::TextAlign::RIGHT:
            metricsFlags |= Qt::AlignRight | Qt::AlignVCenter;
            break;
        case Image::TextAlign::BOTTOM_RIGHT:
            metricsFlags |= Qt::AlignRight | Qt::AlignBottom;
            break;
        case Image::TextAlign::BOTTOM:
            metricsFlags |= Qt::AlignHCenter | Qt::AlignBottom;
            break;
        case Image::TextAlign::BOTTOM_LEFT:
            metricsFlags |= Qt::AlignLeft | Qt::AlignBottom;
            break;
        case Image::TextAlign::LEFT:
            metricsFlags |= Qt::AlignLeft | Qt::AlignVCenter;
            break;
        case Image::TextAlign::TOP_LEFT:
            metricsFlags |= Qt::AlignLeft | Qt::AlignTop;
            break;
        default:
            break;
        }
        return metricsFlags;
    }

    static QString maybeReplaceCustomFontFamily(const QString &fontFamily)
    {
        if (customFonts.contains(fontFamily))
            return customFonts.value(fontFamily);

        const std::string filePath = FileUtils::getInstance()->fullPathForFilename(fontFamily.toStdString());
        if (!FileUtils::getInstance()->isFileExist(filePath)) {
            customFonts[fontFamily] = fontFamily;
            return fontFamily;
        }

        const int databaseId = QFontDatabase::addApplicationFont(QString::fromStdString(filePath));
        if (databaseId == -1) {
            CCLOG("Failed to load font '%s' into QFontDatabase.", filePath.c_str());
            customFonts[fontFamily] = fontFamily;
            return fontFamily;
        }

        QStringList families = QFontDatabase::applicationFontFamilies(databaseId);
        if (families.isEmpty()) {
            CCLOG("Fonts file '%s' is empty.", filePath.c_str());
            customFonts[fontFamily] = fontFamily;
            return fontFamily;
        }

        customFonts[fontFamily] = families.first();
        return families.first();
    }

private:
    static QHash<QString, QString> customFonts;
};

QHash<QString, QString> FontUtilsQt::customFonts;

}

bool Image::initWithString(
		const char * pText,
		int nWidth/* = 0*/,
		int nHeight/* = 0*/,
		TextAlign eAlignMask/* = kAlignCenter*/,
		const char * pFontName/* = nil*/,
		int nSize/* = 0*/)
{
    return initWithStringShadowStroke(pText, nWidth, nHeight, eAlignMask, pFontName, nSize);
}

bool Image::initWithStringShadowStroke(
                                    const char *pText,
                                    int nWidth,
                                    int nHeight,
                                    TextAlign eAlignMask,
                                    const char *pFontName,
                                    int nSize,
                                    float textTintR,
                                    float textTintG,
                                    float textTintB,
                                    bool shadow,
                                    float shadowOffsetX,
                                    float shadowOffsetY,
                                    float shadowOpacity,
                                    float shadowBlur,
                                    bool  stroke,
                                    float strokeR,
                                    float strokeG,
                                    float strokeB,
                                    float strokeSize
                                )
{

    bool bRet = false;
    do
    {
        CC_BREAK_IF(! pText);
        CC_BREAK_IF(! pFontName);

        const QString text = QString::fromLatin1(pText);
        const QString fontFamily = FontUtilsQt::maybeReplaceCustomFontFamily(QString::fromLatin1(pFontName));
        const QColor fontColor(255 * textTintB, 255 * textTintG, 255 * textTintR, 255);
        const int fontFlags = FontUtilsQt::fontFlags(eAlignMask);

        QFont font;
        font.setPixelSize(nSize);
        // FIXME: does it work with custom fonts?
        font.setFamily(fontFamily);

        QFontMetrics metrics(font);
        QRect labelRect = QRect(0, 0, nWidth, nHeight);

        QRect bounds = metrics.boundingRect(labelRect, fontFlags, text);
        QImage canvas(bounds.size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(QColor(0, 0, 0, 0));

        QGraphicsSimpleTextItem item;
        item.setFont(font);
        item.setText(text);
        item.setBrush(QBrush(fontColor));

        QGraphicsDropShadowEffect shadowDrop;
        if (shadow) {
            shadowDrop.setOffset(QPointF(shadowOffsetX, shadowOffsetY));
            shadowDrop.setBlurRadius(shadowBlur);
            shadowDrop.setColor(QColor(0, 0, 0, 255 * shadowOpacity));
            item.setGraphicsEffect(&shadowDrop);
        }

        QStyleOptionGraphicsItem option;
        option.fontMetrics = metrics;

        QPainter painter(&canvas);
        item.paint(&painter, &option, nullptr);
        painter.end();

        _width = canvas.width();
        _height = canvas.height();
        _renderFormat = Texture2D::PixelFormat::RGBA8888;
        _preMulti = true;
        _dataLen = _width * _height * 4;

        const uchar *bits = canvas.constBits();
        _data = new uchar[_dataLen];
        memcpy(_data, bits, _dataLen);

        bRet = true;
    } while (0);

    //do nothing
    return bRet;
}




} // namespace cocos2d
