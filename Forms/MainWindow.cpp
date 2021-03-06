/*
 * This file is part of RaidFinder
 * Copyright (C) 2019-2020 by Admiral_Fish
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include <Core/Generator/RaidGenerator.hpp>
#include <Core/Loader/DenLoader.hpp>
#include <Core/Loader/PersonalLoader.hpp>
#include <Core/Loader/ProfileLoader.hpp>
#include <Core/Util/Translator.hpp>
#include <Forms/Profile/ProfileManager.hpp>
#include <Forms/Tools/DenMap.hpp>
#include <Forms/Tools/EncounterLookup.hpp>
#include <Forms/Tools/IVCalculator.hpp>
#include <Forms/Tools/SeedCalculator.hpp>
#include <Models/FrameModel.hpp>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QThread>

MainWindow::MainWindow(bool debug, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), debug(debug)
{
    ui->setupUi(this);
    setWindowTitle(QString("RaidFinder %1").arg(VERSION));

    PersonalLoader::init();
    DenLoader::init();

    updateProfiles();
    setupModels();
}

MainWindow::~MainWindow()
{
    QSettings setting;
    setting.setValue("mainWindow/profile", ui->comboBoxProfiles->currentIndex());
    setting.setValue("mainWindow/geometry", this->saveGeometry());
    setting.setValue("settings/locale", currentLanguage);
    setting.setValue("settings/style", currentStyle);

    delete ui;
}

void MainWindow::setupModels()
{
    model = new FrameModel(ui->tableView);
    ui->tableView->setModel(model);

    menu = new QMenu(ui->tableView);

    ui->comboBoxNature->setup(Translator::getNatures());

    ui->comboBoxAbilityType->setItemData(0, 0);
    ui->comboBoxAbilityType->setItemData(1, 1);
    ui->comboBoxAbilityType->setItemData(2, 2);
    ui->comboBoxAbilityType->setItemData(3, 3);
    ui->comboBoxAbilityType->setItemData(4, 4);

    ui->textBoxSeed->setValues(InputType::Seed64Bit);
    ui->textBoxInitialFrame->setValues(InputType::Frame32Bit);
    ui->textBoxMaxResults->setValues(InputType::Frame32Bit);

    ui->comboBoxAbility->setItemData(0, 255); // Any ability
    ui->comboBoxAbility->setItemData(1, 0); // Ability 0
    ui->comboBoxAbility->setItemData(2, 1); // Ability 1
    ui->comboBoxAbility->setItemData(3, 2); // Ability 2 (H)

    ui->comboBoxGender->setItemData(0, 255); // Any gender
    ui->comboBoxGender->setItemData(1, 0); // Male
    ui->comboBoxGender->setItemData(2, 1); // Female

    ui->comboBoxGenderRatio->setItemData(0, 255);
    ui->comboBoxGenderRatio->setItemData(1, 0);
    ui->comboBoxGenderRatio->setItemData(2, 254);
    ui->comboBoxGenderRatio->setItemData(3, 31);
    ui->comboBoxGenderRatio->setItemData(4, 63);
    ui->comboBoxGenderRatio->setItemData(5, 127);
    ui->comboBoxGenderRatio->setItemData(6, 191);

    ui->comboBoxShiny->setItemData(0, 255); // Any shiny type
    ui->comboBoxShiny->setItemData(1, 1); // Star
    ui->comboBoxShiny->setItemData(2, 2); // Square
    ui->comboBoxShiny->setItemData(3, 3); // Star or square

    ui->comboBoxShinyType->setItemData(0, 0);
    ui->comboBoxShinyType->setItemData(1, 2);

    ui->spinBoxIVCount->setEnabled(debug);
    ui->comboBoxAbilityType->setEnabled(debug);
    ui->comboBoxGenderType->setEnabled(debug);
    ui->comboBoxGenderRatio->setEnabled(debug);
    ui->comboBoxShinyType->setEnabled(debug);

    for (u8 i = 0; i < 100; i++)
    {
        if (i == 16)
        {
            continue;
        }

        QString location = Translator::getLocation(DenLoader::getLocation(i));
        ui->comboBoxDen->addItem(QString("%1: %2").arg(i + 1).arg(location), i);
    }
    ui->comboBoxDen->addItem(tr("Event"), 100);
    denIndexChanged(0);
    speciesIndexChanged(0);

    QSettings setting;

    languageGroup = new QActionGroup(ui->menuLanguage);
    languageGroup->setExclusive(true);
    connect(languageGroup, &QActionGroup::triggered, this, &MainWindow::slotLanguageChanged);
    currentLanguage = setting.value("settings/locale", "en").toString();
    QStringList locales = { "de", "en", "es", "fr", "it", "ja", "ko", "zh", "tw" };
    for (auto i = 0; i < locales.size(); i++)
    {
        const QString &lang = locales.at(i);

        auto *action = ui->menuLanguage->actions()[i];
        action->setData(lang);

        if (currentLanguage == lang)
        {
            action->setChecked(true);
        }

        languageGroup->addAction(action);
    }

    styleGroup = new QActionGroup(ui->menuStyle);
    styleGroup->setExclusive(true);
    connect(styleGroup, &QActionGroup::triggered, this, &MainWindow::slotStyleChanged);
    currentStyle = setting.value("settings/style", "dark").toString();
    QStringList styles = { "dark", "light" };
    for (auto i = 0; i < styles.size(); i++)
    {
        const QString &style = styles.at(i);

        auto *action = ui->menuStyle->actions()[i];
        action->setData(style);

        if (currentStyle == style)
        {
            action->setChecked(true);
        }

        styleGroup->addAction(action);
    }

    threadGroup = new QActionGroup(ui->menuThread);
    threadGroup->setExclusive(true);
    connect(threadGroup, &QActionGroup::triggered, this, &MainWindow::slotThreadChanged);
    int maxThread = QThread::idealThreadCount();
    int thread = setting.value("settings/thread", maxThread).toInt();
    for (auto i = 1; i <= maxThread; i++)
    {
        auto *action = ui->menuThread->addAction(QString::number(i));
        action->setData(i);
        action->setCheckable(true);

        if (i == thread)
        {
            action->setChecked(true);
        }

        threadGroup->addAction(action);
    }

    QAction *outputTXT = menu->addAction(tr("Output Results to TXT"));
    QAction *outputCSV = menu->addAction(tr("Output Results to CSV"));
    connect(outputTXT, &QAction::triggered, this, [=]() { ui->tableView->outputModelTXT(); });
    connect(outputCSV, &QAction::triggered, this, [=]() { ui->tableView->outputModelCSV(); });

    connect(ui->pushButtonProfileManager, &QPushButton::clicked, this, &MainWindow::openProfileManager);
    connect(ui->actionDenMap, &QAction::triggered, this, &MainWindow::openDenMap);
    connect(ui->actionEncounterLookup, &QAction::triggered, this, &MainWindow::openEncounterLookup);
    connect(ui->actionIVCalculator, &QAction::triggered, this, &MainWindow::openIVCalculator);
    connect(ui->actionSeedSearcher, &QAction::triggered, this, &MainWindow::openSeedSearcher);
    connect(ui->comboBoxProfiles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::profilesIndexChanged);
    connect(ui->pushButtonGenerate, &QPushButton::clicked, this, &MainWindow::generate);
    connect(ui->comboBoxDen, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::denIndexChanged);
    connect(ui->comboBoxRarity, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::rarityIndexChange);
    connect(ui->comboBoxSpecies, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::speciesIndexChanged);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &MainWindow::tableViewContextMenu);

    if (setting.contains("mainWindow/geometry"))
    {
        this->restoreGeometry(setting.value("mainWindow/geometry").toByteArray());
    }
}

void MainWindow::slotLanguageChanged(QAction *action)
{
    if (action)
    {
        QString language = action->data().toString();
        if (currentLanguage != language)
        {
            currentLanguage = language;

            QMessageBox message(QMessageBox::Question, tr("Language update"), tr("Restart for changes to take effect. Restart now?"),
                                QMessageBox::Yes | QMessageBox::No);
            if (message.exec() == QMessageBox::Yes)
            {
                QProcess::startDetached(QApplication::applicationFilePath());
                QApplication::quit();
            }
        }
    }
}

void MainWindow::slotStyleChanged(QAction *action)
{
    if (action)
    {
        QString style = action->data().toString();
        if (currentStyle != style)
        {
            currentStyle = style;

            QMessageBox message(QMessageBox::Question, tr("Style change"), tr("Restart for changes to take effect. Restart now?"),
                                QMessageBox::Yes | QMessageBox::No);
            if (message.exec() == QMessageBox::Yes)
            {
                QProcess::startDetached(QApplication::applicationFilePath());
                QApplication::quit();
            }
        }
    }
}

void MainWindow::slotThreadChanged(QAction *action)
{
    if (action)
    {
        int thread = action->data().toInt();

        QSettings setting;
        setting.setValue("settings/thread", thread);
    }
}

void MainWindow::updateProfiles()
{
    connect(ui->comboBoxProfiles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::profilesIndexChanged);

    profiles = ProfileLoader::getProfiles();
    profiles.insert(profiles.begin(), Profile());

    ui->comboBoxProfiles->clear();

    for (const auto &profile : profiles)
    {
        ui->comboBoxProfiles->addItem(profile.getName());
    }

    QSettings setting;
    int val = setting.value("mainWindow/profile", 0).toInt();
    if (val < ui->comboBoxProfiles->count())
    {
        ui->comboBoxProfiles->setCurrentIndex(val);
    }
}

void MainWindow::profilesIndexChanged(int index)
{
    if (index >= 0)
    {
        currentProfile = profiles.at(index);

        ui->labelProfileTIDValue->setText(QString::number(currentProfile.getTID()));
        ui->labelProfileSIDValue->setText(QString::number(currentProfile.getSID()));
        ui->labelProfileGameValue->setText(currentProfile.getVersionString());

        denIndexChanged(ui->comboBoxDen->currentIndex());
    }
}

void MainWindow::openProfileManager()
{
    auto *manager = new ProfileManager();
    connect(manager, &ProfileManager::updateProfiles, this, &MainWindow::updateProfiles);
    manager->show();
}

void MainWindow::openDenMap()
{
    auto *map = new DenMap();
    map->show();
}

void MainWindow::openEncounterLookup()
{
    auto *lookup = new EncounterLookup();
    lookup->show();
}

void MainWindow::openIVCalculator()
{
    auto *calculator = new IVCalculator();
    calculator->show();
}

void MainWindow::openSeedSearcher()
{
    auto *searcher = new SeedCalculator();
    searcher->show();
}

void MainWindow::denIndexChanged(int index)
{
    if (index >= 0)
    {
        int rarity = ui->comboBoxRarity->currentIndex();
        ui->comboBoxSpecies->clear();
        den = DenLoader::getDen(static_cast<u8>(ui->comboBoxDen->currentData().toInt()), static_cast<u8>(rarity));

        auto raids = den.getRaids(currentProfile.getVersion());
        for (const auto &raid : raids)
        {
            ui->comboBoxSpecies->addItem(QString("%1: %2").arg(Translator::getSpecie(raid.getSpecies()), raid.getStarDisplay()));
        }
    }
}

void MainWindow::rarityIndexChange(int index)
{
    (void)index;
    denIndexChanged(ui->comboBoxDen->currentIndex());
}

void MainWindow::speciesIndexChanged(int index)
{
    if (index >= 0)
    {
        Raid raid = den.getRaid(static_cast<u8>(index), currentProfile.getVersion());
        PersonalInfo info = PersonalLoader::getInfo(raid.getSpecies(), raid.getAltForm());

        ui->spinBoxIVCount->setValue(raid.getIVCount());
        ui->comboBoxAbilityType->setCurrentIndex(ui->comboBoxAbilityType->findData(raid.getAbility()));
        ui->comboBoxGenderType->setCurrentIndex(raid.getGender());
        ui->comboBoxGenderRatio->setCurrentIndex(ui->comboBoxGenderRatio->findData(raid.getGenderRatio()));
        ui->comboBoxShinyType->setCurrentIndex(ui->comboBoxShinyType->findData(raid.getShiny()));
        ui->labelGigantamaxValue->setText(raid.getGigantamax() ? tr("Yes") : tr("No"));

        ui->comboBoxAbility->setItemText(1, "1: " + Translator::getAbility(info.getAbility1()));
        ui->comboBoxAbility->setItemText(2, "2: " + Translator::getAbility(info.getAbility2()));
        if (raid.getAbility() == 4)
        {
            ui->comboBoxAbility->setItemText(3, "H: " + Translator::getAbility(info.getAbilityH()));
        }
    }
}

void MainWindow::tableViewContextMenu(QPoint pos)
{
    if (model->rowCount() == 0)
    {
        return;
    }

    menu->popup(ui->tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::generate()
{
    Raid raid = den.getRaid(static_cast<u8>(ui->comboBoxSpecies->currentIndex()), currentProfile.getVersion());
    model->clearModel();
    model->setInfo(PersonalLoader::getInfo(raid.getSpecies(), raid.getAltForm()));

    u32 initialFrame = ui->textBoxInitialFrame->getUInt();
    u32 maxResults = ui->textBoxMaxResults->getUInt();
    u16 tid = currentProfile.getTID();
    u16 sid = currentProfile.getSID();

    RaidGenerator *generator;
    if (debug)
    {
        u8 abilityType = static_cast<u8>(ui->comboBoxAbilityType->currentData().toInt());
        u8 genderType = static_cast<u8>(ui->comboBoxGenderType->currentIndex());
        u8 genderRatio = static_cast<u8>(ui->comboBoxGenderRatio->currentData().toInt());
        u8 ivCount = static_cast<u8>(ui->spinBoxIVCount->value());
        u8 shinyType = static_cast<u8>(ui->comboBoxShinyType->currentData().toInt());

        generator = new RaidGenerator(initialFrame, maxResults, tid, sid, raid.getSpecies(), abilityType, shinyType, ivCount, genderType,
                                      genderRatio);
    }
    else
    {
        generator = new RaidGenerator(initialFrame, maxResults, tid, sid, raid);
    }

    u8 gender = static_cast<u8>(ui->comboBoxGender->currentData().toInt());
    u8 ability = static_cast<u8>(ui->comboBoxAbility->currentData().toInt());
    u8 shiny = static_cast<u8>(ui->comboBoxShiny->currentData().toInt());
    bool skip = ui->checkBoxDisableFilters->isChecked();
    QVector<u8> min = ui->ivFilter->getLower();
    QVector<u8> max = ui->ivFilter->getUpper();
    QVector<bool> natures = ui->comboBoxNature->getChecked();
    FrameFilter filter(gender, ability, shiny, skip, min, max, natures);

    u64 seed = ui->textBoxSeed->getULong();

    QVector<Frame> frames = generator->generate(filter, seed);
    model->addItems(frames);

    delete generator;
}
