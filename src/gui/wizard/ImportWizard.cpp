/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "ImportWizard.h"
#include "ImportWizardPageReview.h"
#include "ImportWizardPageSelect.h"

#include "core/Global.h"
#include "core/Group.h"

#include <QFrame>
#include <QPalette>

ImportWizard::ImportWizard(QWidget* parent)
    : QWizard(parent)
    , m_pageSelect(new ImportWizardPageSelect)
    , m_pageReview(new ImportWizardPageReview)
{
    setWizardStyle(MacStyle);
    setOption(HaveHelpButton, false);
    setOption(NoDefaultButton, false); // Needed for macOS

    addPage(m_pageSelect.data());
    addPage(m_pageReview.data());

    setWindowTitle(tr("Import Wizard"));

    Q_INIT_RESOURCE(wizard);
    setPixmap(BackgroundPixmap, QPixmap(":/wizard/background-pixmap.png"));

    // Fix MacStyle QWizard page frame too bright in dark mode (QTBUG-70346, QTBUG-71696)
    QPalette defaultPalette;
    auto windowColor = defaultPalette.color(QPalette::Window);
    windowColor.setAlpha(153);
    auto baseColor = defaultPalette.color(QPalette::Base);
    baseColor.setAlpha(153);

    auto* pageFrame = findChildren<QFrame*>()[0];
    auto framePalette = pageFrame->palette();
    framePalette.setBrush(QPalette::Window, windowColor.lighter(120));
    framePalette.setBrush(QPalette::Base, baseColor.lighter(120));
    pageFrame->setPalette(framePalette);
}

ImportWizard::~ImportWizard()
{
}

bool ImportWizard::validateCurrentPage()
{
    bool ret = QWizard::validateCurrentPage();
    if (ret && currentPage() == m_pageReview) {
        m_db = m_pageReview->database();
    }
    return ret;
}

QPair<QUuid, QUuid> ImportWizard::importInto()
{
    auto list = field("ImportInto").toList();
    if (list.size() >= 2) {
        return qMakePair(QUuid(list[0].toString()), QUuid(list[1].toString()));
    }
    return {};
}

QSharedPointer<Database> ImportWizard::database()
{
    return m_db;
}
