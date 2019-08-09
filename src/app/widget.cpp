#include <QVersionNumber>
#include <QLineEdit>
#include <QDir>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QFileDialog>
#include <QDateTime>

#include "widget.h"
#include "ui_widget.h"
#include "dirutils.h"
#include "fileutils.h"
#include "patcher.h"

Widget::Widget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // Set window title.
    setWindowTitle(QString("%1 %2").arg(app_name, QVersionNumber(app_version_major, app_version_minor, app_version_micro).toString()));

    // Set label text.
    ui->label_installation_directory->setText(tr("Select the %1 installation directory:").arg(game_name));

    // Add placeholder text to lineEdit.
    QLineEdit *lineEdit_install_directory = ui->comboBox_install_directory->lineEdit();
    lineEdit_install_directory->setPlaceholderText(tr("Enter path to install directory..."));

    // Populate comboBox with found install directories.
    populateComboboxWithInstallDirectories();

    // Populate comboBox with detected network interfaces.
    populateComboboxWithNetworkInterfaces();

    // Load settings from configuration file.
    settings = new QSettings(app_configuration_file, QSettings::IniFormat, this);
    loadSettings();

    // Update patch button according to patch status.
    bool patched = Patcher::isPatched(getInstallDirectory(false));
    updatePatchStatus(patched);

    // Register GUI signals to slots.
    connect(ui->pushButton_install_directory,   &QPushButton::clicked,                                  this, &Widget::pushButton_install_directory_clicked);

    connect(ui->comboBox_network_interface,     QOverload<int>::of(&QComboBox::currentIndexChanged),    this, &Widget::comboBox_network_interface_currentIndexChanged);
    connect(ui->pushButton_patch,               &QPushButton::clicked,                                  this, &Widget::pushButton_patch_clicked);

    // Register signals to saveSettings slot.
    connect(ui->comboBox_install_directory,     QOverload<int>::of(&QComboBox::currentIndexChanged),    this, &Widget::saveSettings);
    connect(ui->comboBox_network_interface,     QOverload<int>::of(&QComboBox::currentIndexChanged),    this, &Widget::saveSettings);
    connect(ui->pushButton_patch,               &QPushButton::clicked,                                  this, &Widget::saveSettings);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::closeEvent(QCloseEvent* event)
{
    saveSettings();

    QWidget::closeEvent(event);
}

void Widget::loadSettings()
{
    QDir dir = settings->value(settings_install_directory).toString();

    if (DirUtils::isGameDirectory(dir)) {
        const QStringList &installDirectories = DirUtils::findInstallDirectories();

        // If we're in executable directory, cd up to install directory.
        if (dir.dirName() == game_executable_directory) {
            dir.cdUp();
        }

        // Avoid duplicate install directories.
        if (!installDirectories.contains(dir.absolutePath())) {
            ui->comboBox_install_directory->insertItem(0, dir.absolutePath());
        }
    }

    int index = settings->value(settings_interface_index).toInt();

    // Only set valid index in UI.
    if (index < ui->comboBox_network_interface->count()) {
        ui->comboBox_network_interface->setCurrentIndex(index);
    }

    settings->beginGroup(settings_group_window);
        resize(settings->value(settings_group_window_size, size()).toSize());
        move(settings->value(settings_group_window_position, pos()).toPoint());

        if (settings->value(settings_group_window_isMaximized, false).toBool()) {
            showMaximized();
        }
    settings->endGroup();
}

void Widget::saveSettings() const
{
    QString path = ui->comboBox_install_directory->currentText();

    if (DirUtils::isGameDirectory(path)) {
        settings->setValue(settings_install_directory, path);
    }

    settings->setValue(settings_interface_index, ui->comboBox_network_interface->currentIndex());
    settings->beginGroup(settings_group_window);
        settings->setValue(settings_group_window_size, size());
        settings->setValue(settings_group_window_position, pos());
        settings->setValue(settings_group_window_isMaximized, isMaximized());
    settings->endGroup();
}

QString Widget::getInstallDirectory(bool warning)
{
    // Create path to binary folder.
    QDir dir = ui->comboBox_install_directory->currentText();

    if (DirUtils::isGameDirectory(dir)) {
        return dir.absolutePath();
    }

    if (warning) {
        QMessageBox::warning(this, "Warning", tr("%1 installation directory not found, please select it manually.").arg(game_name));
    }

    return QString();
}

void Widget::populateComboboxWithInstallDirectories() const
{
    for (const QString &path : DirUtils::findInstallDirectories()) {
        ui->comboBox_install_directory->addItem(path);
    }
}

void Widget::populateComboboxWithNetworkInterfaces() const
{
    const QList<QNetworkInterface> &interfaces = QNetworkInterface::allInterfaces();

    // If no network interfaces is found, return early.
    if (interfaces.isEmpty()) {
        ui->comboBox_network_interface->setEnabled(false);
        ui->comboBox_network_interface->addItem(tr("No network interfaces found."));

        return;
    }

    // Loop thru all of the systems network interfaces.
    for (const QNetworkInterface &interface : interfaces) {
        const QNetworkInterface::InterfaceFlags &flags = interface.flags();

        // Only show active network interfaces and not loopback interfaces.
        if (flags.testFlag(QNetworkInterface::IsUp) && !flags.testFlag(QNetworkInterface::IsLoopBack)) {
            QNetworkAddressEntry selectedAddressEntry;

            // Scan thru addresses for this interface.
            for (const QNetworkAddressEntry &addressEntry : interface.addressEntries()) {
                // Only select first IPv4 address found.
                if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    selectedAddressEntry = addressEntry;
                    break;
                }
            }

            // TODO: Handle this if it's empty.
            ui->comboBox_network_interface->addItem(interface.humanReadableName() + " (" + selectedAddressEntry.ip().toString() + ")", QVariant::fromValue<QNetworkAddressEntry>(selectedAddressEntry));
        }
    }
}

void Widget::updatePatchStatus(bool patched) const
{
    ui->pushButton_patch->setText(!patched ? tr("Install patch") : tr("Uninstall patch"));
}

void Widget::pushButton_install_directory_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select the %1 installation directory").arg(game_name), ui->comboBox_install_directory->currentText(), QFileDialog::ReadOnly);

    if (DirUtils::isGameDirectory(path)) {
        ui->comboBox_install_directory->setCurrentText(path);
    }
}

void Widget::comboBox_network_interface_currentIndexChanged(int index)
{
    QDir dir = getInstallDirectory();

    log(tr("Generated network configuration, saved to: %1").arg(dir.absolutePath()));

    // Only update network configuration if game is patched.
    if (Patcher::isPatched(dir)) {
        // Generate network configuration.
        Patcher::generateNetworkConfigFile(dir, ui->comboBox_network_interface->itemData(index).value<QNetworkAddressEntry>());
    }
}

void Widget::pushButton_patch_clicked()
{
    // Create path to binary folder.
    QDir dir = getInstallDirectory();
    dir.cd(game_executable_directory);

    // Only show option to patch if not already patched.
    if (Patcher::isPatched(dir)) {
        Patcher::undoPatch(dir);

        updatePatchStatus(false);
    } else {
        // Apply patch to files, if successful continue.
        if (Patcher::patch(this, dir)) {
            // Generate network configuration.
            Patcher::generateNetworkConfigFile(dir, ui->comboBox_network_interface->currentData().value<QNetworkAddressEntry>());

            updatePatchStatus(true);
        }
    }
}

void Widget::log(const QString &message)
{
    ui->textEdit->append(QString("<span style=\"color:grey;\">[%1]</span>: %2").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"), message));
}

void Widget::on_pushButton_clicked()
{
    bool visible = ui->textEdit->isVisible();

    ui->pushButton->setText(visible ? "Show" : "Hide");
    ui->textEdit->setVisible(!visible);
}
