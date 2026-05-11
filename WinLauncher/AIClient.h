#ifndef AICLIENT_H
#define AICLIENT_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct AIConfig {
    QString apiKey;
    QString model = "claude-sonnet-4-20250514";
    int maxTokens = 1024;
    double temperature = 0.7;
};

class AIClient : public QObject
{
    Q_OBJECT

public:
    explicit AIClient(QObject *parent = nullptr);
    void setConfig(const AIConfig &config);
    bool isReady() const;
    void sendPrompt(const QString &prompt, const QString &systemPrompt = QString());
    void cancel();

signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &reason);
    void finished();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QJsonDocument buildRequestBody(const QString &prompt, const QString &systemPrompt);
    QString parseResponse(const QByteArray &data);
    QUrl buildApiUrl() const;

    QNetworkAccessManager *m_network;
    AIConfig m_config;
    QNetworkReply *m_currentReply = nullptr;
};

#endif // AICLIENT_H
