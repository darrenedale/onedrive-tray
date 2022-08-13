//
// Created by darren on 13/08/22.
//

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QDialogButtonBox>

#include "SettingsWindow.h"
#include "SettingsWidget.h"

using namespace OneDrive;


SettingsWindow::SettingsWindow(const Settings & settings, QWidget * parent)
: QDialog(parent),
  m_settingsWidget(std::make_unique<SettingsWidget>(settings))
{
    auto * layout = new QVBoxLayout(this);
    layout->addWidget(m_settingsWidget.get());

    auto * buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal,  this);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::close);

    setLayout(layout);
}


SettingsWindow::~SettingsWindow() noexcept = default;
