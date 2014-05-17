/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "DatabaseWidgetStateSync.h"

#include "core/Config.h"

DatabaseWidgetStateSync::DatabaseWidgetStateSync(QObject* parent)
    : QObject(parent)
    , m_activeDbWidget(Q_NULLPTR)
{
    QVariantList variantList = config()->get("GUI/SplitterState").toList();
    Q_FOREACH (const QVariant& var, variantList) {
        bool ok;
        int size = var.toInt(&ok);
        if (ok) {
            m_splitterSizes.append(size);
        }
        else {
            m_splitterSizes.clear();
            break;
        }
    }
}

DatabaseWidgetStateSync::~DatabaseWidgetStateSync()
{
    QVariantList variantList;
    Q_FOREACH (int size, m_splitterSizes) {
        variantList.append(size);
    }

    config()->set("GUI/SplitterState", variantList);
}

void DatabaseWidgetStateSync::setActive(DatabaseWidget* dbWidget)
{
    if (m_activeDbWidget) {
        disconnect(m_activeDbWidget, 0, this, 0);
    }

    m_activeDbWidget = dbWidget;

    if (m_activeDbWidget) {
        if (!m_splitterSizes.isEmpty()) {
            m_activeDbWidget->setSplitterSizes(m_splitterSizes);
        }

        connect(m_activeDbWidget, SIGNAL(splitterSizesChanged()),
                SLOT(updateSplitterSizes()));
    }
}

void DatabaseWidgetStateSync::updateSplitterSizes()
{
    m_splitterSizes = m_activeDbWidget->splitterSizes();
}
