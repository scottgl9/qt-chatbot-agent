/**
 * SSEClient.cpp - Server-Sent Events (SSE) client
 * 
 * Handles streaming responses from SSE endpoints, parses event data,
 * and manages connection lifecycle for real-time LLM responses.
 */

#include "SSEClient.h"
#include "Logger.h"
#include <QTimer>
#include <QUrl>

SSEClient::SSEClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_currentRetry(-1) {

    // Defer network manager creation
    QTimer::singleShot(0, this, [this]() {
        m_networkManager = new QNetworkAccessManager(this);
        LOG_DEBUG("SSEClient: QNetworkAccessManager initialized");
    });
}

SSEClient::~SSEClient() {
    disconnect();
}

void SSEClient::connectToStream(const QString &url, const QString &lastEventId) {
    if (m_currentReply) {
        LOG_WARNING("SSEClient: Already connected, disconnecting first");
        disconnect();
    }

    if (!m_networkManager) {
        LOG_ERROR("SSEClient: Network manager not initialized");
        emit errorOccurred("Network manager not initialized");
        return;
    }

    m_streamUrl = url;
    m_lastEventId = lastEventId;

    LOG_INFO(QString("SSEClient: Connecting to %1").arg(url));

    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");

    // Add Last-Event-ID header if provided (for reconnection)
    if (!lastEventId.isEmpty()) {
        request.setRawHeader("Last-Event-ID", lastEventId.toUtf8());
        LOG_DEBUG(QString("SSEClient: Reconnecting from event ID: %1").arg(lastEventId));
    }

    m_currentReply = m_networkManager->get(request);

    // Connect signals
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, &SSEClient::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &SSEClient::handleFinished);
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &SSEClient::handleError);

    // Reset parsing state
    resetEventBuffer();

    emit connected(url);
}

void SSEClient::disconnect() {
    if (m_currentReply) {
        LOG_INFO("SSEClient: Disconnecting from stream");
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        emit disconnected();
    }

    m_streamUrl.clear();
    resetEventBuffer();
}

bool SSEClient::isConnected() const {
    return m_currentReply != nullptr && m_currentReply->isRunning();
}

QString SSEClient::getStreamUrl() const {
    return m_streamUrl;
}

void SSEClient::handleReadyRead() {
    if (!m_currentReply) {
        return;
    }

    QByteArray newData = m_currentReply->readAll();
    m_buffer.append(newData);

    // Process complete events (separated by \n\n)
    while (m_buffer.contains("\n\n")) {
        int eventEnd = m_buffer.indexOf("\n\n");
        QByteArray eventData = m_buffer.left(eventEnd);
        m_buffer = m_buffer.mid(eventEnd + 2);

        parseSSEData(eventData);
    }
}

void SSEClient::handleFinished() {
    if (!m_currentReply) {
        return;
    }

    LOG_INFO("SSEClient: Stream finished");

    // Process any remaining data
    if (!m_buffer.isEmpty()) {
        parseSSEData(m_buffer);
        m_buffer.clear();
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    emit disconnected();
}

void SSEClient::handleError(QNetworkReply::NetworkError error) {
    if (!m_currentReply) {
        return;
    }

    QString errorMsg = QString("SSE error: %1 (%2)")
        .arg(m_currentReply->errorString())
        .arg(error);

    LOG_ERROR(errorMsg);
    emit errorOccurred(errorMsg);
}

void SSEClient::parseSSEData(const QByteArray &data) {
    QString dataStr = QString::fromUtf8(data);
    QStringList lines = dataStr.split('\n');

    for (const QString &line : lines) {
        // Skip empty lines
        if (line.isEmpty()) {
            continue;
        }

        // Comments (lines starting with :)
        if (line.startsWith(':')) {
            LOG_DEBUG(QString("SSE comment: %1").arg(line.mid(1)));
            continue;
        }

        // Parse field
        int colonPos = line.indexOf(':');
        if (colonPos == -1) {
            // Field with no value
            LOG_WARNING(QString("SSE: Invalid line (no colon): %1").arg(line));
            continue;
        }

        QString field = line.left(colonPos);
        QString value = line.mid(colonPos + 1);

        // Remove optional space after colon
        if (value.startsWith(' ')) {
            value = value.mid(1);
        }

        // Process field
        if (field == "event") {
            m_currentEventType = value;
        } else if (field == "data") {
            if (!m_currentData.isEmpty()) {
                m_currentData.append('\n');
            }
            m_currentData.append(value);
        } else if (field == "id") {
            m_currentEventId = value;
        } else if (field == "retry") {
            bool ok;
            int retryMs = value.toInt(&ok);
            if (ok) {
                m_currentRetry = retryMs;
            }
        } else {
            LOG_DEBUG(QString("SSE: Unknown field '%1': %2").arg(field, value));
        }
    }

    // Dispatch event if we have data
    if (!m_currentData.isEmpty() || !m_currentEventType.isEmpty() || !m_currentEventId.isEmpty()) {
        SSEEvent event;
        event.eventType = m_currentEventType.isEmpty() ? "message" : m_currentEventType;
        event.data = m_currentData;
        event.id = m_currentEventId;
        event.retry = m_currentRetry;

        processSSEEvent(event);
        resetEventBuffer();
    }
}

void SSEClient::processSSEEvent(const SSEEvent &event) {
    LOG_DEBUG(QString("SSE Event - Type: %1, ID: %2, Data length: %3")
              .arg(event.eventType, event.id).arg(event.data.length()));

    // Store last event ID for potential reconnection
    if (!event.id.isEmpty()) {
        m_lastEventId = event.id;
    }

    emit eventReceived(event);
}

void SSEClient::resetEventBuffer() {
    m_currentEventType.clear();
    m_currentEventId.clear();
    m_currentData.clear();
    m_currentRetry = -1;
}
