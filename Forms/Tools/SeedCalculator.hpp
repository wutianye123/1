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

#ifndef SEEDCALCULATOR_HPP
#define SEEDCALCULATOR_HPP

#include <Core/Results/Den.hpp>
#include <QWidget>

namespace Ui
{
    class SeedCalculator;
}

class SeedCalculator : public QWidget
{
    Q_OBJECT

public:
    explicit SeedCalculator(QWidget *parent = nullptr);
    ~SeedCalculator() override;

private:
    Ui::SeedCalculator *ui;
    Den den;

    void setupModels();
    QVector<u8> getIVs(const QVector<u8> &ivs1, const QVector<u8> &ivs2);
    void checkDay4();
    void toggleControls(bool flag);

private slots:
    void denIndexChanged(int index);
    void rarityIndexChanged(int index);
    void gameIndexChanged(int index);
    void raidDay4_1IndexChanged(int index);
    void raidDay4_2IndexChanged(int index);
    void raidDay5IndexChanged(int index);
    void search();
    void clear();
};

#endif // SEEDCALCULATOR_HPP
