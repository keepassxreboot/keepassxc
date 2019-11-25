/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MarkdownViewerDialog.h"
#include "ui_MarkdownViewerDialog.h"

#include "MarkdownViewerTransferObject.h"

static const QString sourceMarkdownStylesheet = "https://cdnjs.cloudflare.com/ajax/libs/github-markdown-css/3.0.1/github-markdown.css";
static const QString sourceMarkdownStylesheetHash = "sha256-jyWGtg7ocpUge8Zt/LotwFtPMWE23n7jgkHHw/Ejh+U=";
static const QString sourceMarkdownParser = "https://cdnjs.cloudflare.com/ajax/libs/marked/0.7.0/marked.js";
static const QString sourceMarkdownParserHash = "sha256-K7OfhKvtqBJ0+5ngAlWG+5sJrlwDOC4OCWj+nP7qOnw=";

static const QString documentFoundation = R"(
<!DOCTYPE html>
<html>
<head>
    <title>KeePassXC</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" type="text/css" href="%1" integrity="%2" crossorigin="anonymous" />
    <script type="application/javascript" src="%3" integrity="%4" crossorigin="anonymous"></script>
    <script type="application/javascript" src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <style>
        .markdown-body {
            box-sizing: border-box;
            min-width: 200px;
            max-width: 980px;
            margin: 0 auto;
            padding: 45px;
        }

        @media (max-width: 767px) {
            .markdown-body {
                padding: 15px;
            }
        }
    </style>
</head>
<body>
    <div id="placeholder" class="markdown-body"></div>
    <script>
        "use strict";

        new QWebChannel(qt.webChannelTransport,
            function(channel) {
                var transferObject = channel.objects.transferObject; // Retrieves the object named 'transferObject'.
                var markdown = transferObject.markdownProperty;
                var html = marked(markdown); // Parses the markdown content.
                document.getElementById("placeholder").innerHTML = html;
            }
        );
    </script>
</body>
</html>
)";

static const QString documentFailedProcessingNetworkReply = R"(
<!DOCTYPE html>
<html>
<head>
    <title>KeePassXC</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
    <p>Could not fetch content</p>
</body>
</html>
)";

MarkdownViewerDialog::MarkdownViewerDialog(QUrl urlToMarkdownContent, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::MarkdownViewerDialog())
{
    QNetworkRequest request;

    m_page = new MarkdownViewerPage();

    m_ui->setupUi(this);
    m_ui->canvasWebEngineView->setPage(m_page);

    request.setUrl(urlToMarkdownContent);

    m_reply = getNetMgr()->get(request);

    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &MarkdownViewerDialog::errorProcessingNetworkReply);
    connect(m_reply, SIGNAL(finished()), this, SLOT(finishedProcessingNetworkReply()));
    connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(finishedLoadingContent(bool)));
    connect(m_ui->closeButtonBox, SIGNAL(rejected()), SLOT(close()));
}

MarkdownViewerDialog::~MarkdownViewerDialog()
{
}

void MarkdownViewerDialog::errorProcessingNetworkReply(QNetworkReply::NetworkError code)
{
    qWarning() << code;

    m_page->setHtml(documentFailedProcessingNetworkReply);
}

void MarkdownViewerDialog::finishedProcessingNetworkReply()
{
    QNetworkReply::NetworkError networkErrorStatusCode;

    if((networkErrorStatusCode = m_reply->error()) != QNetworkReply::NoError) {
        qWarning() << networkErrorStatusCode;
        return;
    }

    QByteArray markdown = m_reply->readAll();
    m_reply->deleteLater();
    MarkdownViewerTransferObject* transferObject(new MarkdownViewerTransferObject(markdown));

    QWebChannel* channel(new QWebChannel(this));
    m_page->setWebChannel(channel);
    channel->registerObject(QStringLiteral("transferObject"), transferObject); // Exposes the object to the JavaScript side under the name 'transferObject'.

    m_page->setHtml(documentFoundation.arg(sourceMarkdownStylesheet).arg(sourceMarkdownStylesheetHash).arg(sourceMarkdownParser).arg(sourceMarkdownParserHash));
}

void MarkdownViewerDialog::finishedLoadingContent(bool isLoadSuccessful)
{
    if(!isLoadSuccessful) {
        qWarning() << "Load operation failed";
        return;
    }
}
