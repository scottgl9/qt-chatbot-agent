/**
 * SSEClient.h - Server-Sent Events (SSE) client
 * 
 * Handles streaming HTTP responses from SSE endpoints, parses event data,
 * and manages connection lifecycle for real-time LLM streaming.
 */

#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QByteArray>
#include <QMap>

/**
 * @brief SSE (Server-Sent Events) Client for MCP
 *
 * Implements SSE protocol for real-time streaming from MCP servers.
 * Supports standard SSE events with id, event, data, and retry fields.
 */
class SSEClient : public QObject {
    Q_OBJECT

public:
    /**
     * @brief SSE Event structure
     */
    struct SSEEvent {
        QString id;           // Event ID (for reconnection)
        QString eventType;    // Event type (default: "message")
        QString data;         // Event data (can be multiline)
        int retry;           // Reconnection time in milliseconds (-1 if not set)

        SSEEvent() : retry(-1) {}
    };

    explicit SSEClient(QObject *parent = nullptr);
    ~SSEClient();

    /**
     * @brief Connect to an SSE endpoint
     * @param url The SSE endpoint URL
     * @param lastEventId Optional: Last event ID for reconnection
     */
    void connectToStream(const QString &url, const QString &lastEventId = QString());

    /**
     * @brief Disconnect from the SSE stream
     */
    void disconnect();

    /**
     * @brief Check if currently connected
     * @return True if connected to a stream
     */
    bool isConnected() const;

    /**
     * @brief Get the current stream URL
     * @return The URL of the current stream
     */
    QString getStreamUrl() const;

signals:
    /**
     * @brief Emitted when an SSE event is received
     * @param event The SSE event
     */
    void eventReceived(const SSEEvent &event);

    /**
     * @brief Emitted when successfully connected
     * @param url The stream URL
     */
    void connected(const QString &url);

    /**
     * @brief Emitted when disconnected
     */
    void disconnected();

    /**
     * @brief Emitted on connection error
     * @param error Error message
     */
    void errorOccurred(const QString &error);

private slots:
    void handleReadyRead();
    void handleFinished();
    void handleError(QNetworkReply::NetworkError error);

private:
    void parseSSEData(const QByteArray &data);
    void processSSEEvent(const SSEEvent &event);
    void resetEventBuffer();

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_streamUrl;
    QString m_lastEventId;

    // SSE parsing state
    QByteArray m_buffer;
    QString m_currentEventType;
    QString m_currentEventId;
    QString m_currentData;
    int m_currentRetry;
};

#endif // SSECLIENT_H
