#include "SettingsWidget.h"
#include "ui/ui_settingswidget.h"
#include "Settings.h"

using namespace OneDrive;

SettingsWidget::SettingsWidget(const Settings & settings, QWidget *parent) :
    QWidget(parent),
    m_ui(std::make_unique<Ui::SettingsWidget>())
{
    m_ui->setupUi(this);
    connectComponents();

    if (settings.startOwnOneDrive()) {
        m_ui->startOwnOneDrive->setChecked(true);
    } else {
        m_ui->connectToExistingOneDrive->setChecked(true);
    }

    m_ui->useCustomOneDrive->setChecked(settings.useCustomOneDrive());
    m_ui->customOneDrivePath->setText(QString::fromStdString(settings.customOneDrivePath()));
    m_ui->useCustomSocket->setChecked(settings.useCustomSocket());
    m_ui->customSocketPath->setText(QString::fromStdString(settings.customSocketPath()));
    synchroniseWidgetStates();
}


SettingsWidget::~SettingsWidget() = default;


bool SettingsWidget::startOwnOneDrive() const
{
    return m_ui->startOwnOneDrive->isChecked();
}


void SettingsWidget::setStartOwnOneDrive(bool start)
{
    m_ui->startOwnOneDrive->setChecked(start);
}


bool SettingsWidget::useCustomOneDrive() const
{
    return m_ui->useCustomOneDrive->isChecked();
}


void SettingsWidget::setUseCustomOneDrive(bool use)
{
    m_ui->useCustomOneDrive->setChecked(use);
}


QString SettingsWidget::customOneDrivePath() const
{
    return m_ui->customOneDrivePath->text();
}


void SettingsWidget::setCustomOneDrivePath(const QString & exe)
{
    return m_ui->customOneDrivePath->setText(exe);
}


[[maybe_unused]] bool SettingsWidget::useCustomSocket() const
{
    return m_ui->useCustomSocket->isChecked();
}


void SettingsWidget::setUseCustomSocket(bool use)
{
    m_ui->useCustomSocket->setChecked(use);
}


QString SettingsWidget::customSocketPath() const
{
    return m_ui->customSocketPath->text();
}


void SettingsWidget::setCustomSocketPath(const QString & path)
{
    return m_ui->customSocketPath->setText(path);
}


void SettingsWidget::connectComponents()
{
    connect(m_ui->startOwnOneDrive, &QRadioButton::toggled, this, &SettingsWidget::synchroniseWidgetStates);
    connect(m_ui->useCustomSocket, &QCheckBox::toggled, this, &SettingsWidget::synchroniseWidgetStates);
    connect(m_ui->useCustomOneDrive, &QCheckBox::toggled, this, &SettingsWidget::synchroniseWidgetStates);

    connect(m_ui->startOwnOneDrive, &QRadioButton::toggled, this, &SettingsWidget::changed);
    connect(m_ui->useCustomOneDrive, &QCheckBox::toggled, this, &SettingsWidget::changed);
    connect(m_ui->useCustomSocket, &QCheckBox::toggled, this, &SettingsWidget::changed);
    connect(m_ui->customOneDrivePath, &QLineEdit::textChanged, this, &SettingsWidget::changed);
    connect(m_ui->customSocketPath, &QLineEdit::textChanged, this, &SettingsWidget::changed);
}


void SettingsWidget::synchroniseWidgetStates()
{
    if (m_ui->startOwnOneDrive->isChecked()) {
        m_ui->useCustomOneDrive->setEnabled(true);
        m_ui->customOneDrivePath->setEnabled(m_ui->useCustomOneDrive->isChecked());
        m_ui->chooseCustomOneDrivePath->setEnabled(m_ui->useCustomOneDrive->isChecked());
    } else {
        m_ui->useCustomOneDrive->setEnabled(false);
        m_ui->customOneDrivePath->setEnabled(false);
        m_ui->chooseCustomOneDrivePath->setEnabled(false);
    }


    m_ui->customSocketPath->setEnabled(m_ui->useCustomSocket->isChecked());
    m_ui->chooseCustomSocketPath->setEnabled(m_ui->useCustomSocket->isChecked());
}
