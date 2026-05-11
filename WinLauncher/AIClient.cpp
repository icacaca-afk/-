#include "AIClient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QDebug>

static const QString ANTHROPIC_API_URL = "https://api.anthropic.com/v1/messages";
static const QString API_VERSION = "2023-06-01";

AIClient::AIClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void AIClient::setConfig(const AIConfig &config)
{
    m_config = config;
}

bool AIClient::isReady() const
{
    return !m_config.apiKey.isEmpty();
}

void AIClient::sendPrompt(const QString &prompt, const QString &systemPrompt)
{
    if (!isReady()) {
        emit errorOccurred("API Key 未配置");
        return;
    }

    if (m_currentReply && m_currentReply->isRunning()) {
        cancel();
    }

    QUrl url = buildApiUrl();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", m_config.apiKey.toUtf8());
    request.setRawHeader("anthropic-version", API_VERSION.toUtf8());

    QJsonDocument body = buildRequestBody(prompt, systemPrompt);

    m_currentReply = m_network->post(request, body.toJson(QJson::Compact));
    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        onReplyFinished(m_currentReply);
    });
}

void AIClient::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void AIClient::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply != m_currentReply) return;
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QString("网络错误: %1").arg(reply->errorString()));
        emit finished();
        return;
    }

    QByteArray data = reply->readAll();
    QString responseText = parseResponse(data);

    if (!responseText.isEmpty()) {
        emit responseReady(responseText);
    } else {
        emit errorOccurred("AI 返回了空响应或解析失败");
    }

    emit finished();
}

QJsonDocument AIClient::buildRequestBody(const QString &prompt, const QString &systemPrompt)
{
    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = prompt;

    QJsonArray messages;
    messages.append(messageObj);

    QJsonObject body;
    body["model"] = m_config.model;
    body["max_tokens"] = m_config.maxTokens;
    body["temperature"] = m_config.temperature;
    body["messages"] = messages;

    if (!systemPrompt.isEmpty()) {
        body["system"] = systemPrompt;
    }

    return QJsonDocument(body);
}

QString AIClient::parseResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "AI 响应 JSON 解析失败:" << parseError.errorString();
        return QString();
    }

    QJsonObject root = doc.object();

    // 检查错误字段
    if (root.contains("error")) {
        QJsonObject errObj = root["error"].toObject();
        QString errMsg = errObj["message"].toString();
        qWarning() << "AI API 错误:" << errMsg;
        emit errorOccurred(errMsg.isEmpty() ? "未知 AI API 错误" : errMsg);
        return QString();
    }

    // 提取内容
    if (root.contains("content")) {
        QJsonArray content = root["content"].toArray();
        QStringList textParts;
        for (const QJsonValue &val : content) {
            QJsonObject block = val.toObject();
            if (block["type"].toString() == "text") {
                textParts.append(block["text"].toString());
            }
        }
        return textParts.join("\n");
    }

    return QString();
}

QUrl AIClient::buildApiUrl() const
{
    return QUrl(ANTHROPIC_API_URL);
}
