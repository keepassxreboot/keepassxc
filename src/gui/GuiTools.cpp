#include "GuiTools.h"

namespace GuiTools
{
    void buildDocumentFromMarkup(QTextDocument* doc, QString markup)
    {
        doc->setHtml(markup.replace("\n", "<br/>"));
    }
} // namespace GuiTools
