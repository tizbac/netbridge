/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ROUTINGMANAGER_H
#define ROUTINGMANAGER_H
#include <string>
class RoutingManager
{
public:
    RoutingManager();
    ~RoutingManager();
    //void clearRoutingTable(std::string table);
    void addDefaultRouteToIface(std::string iface , std::string default_route , std::string table);
    void removeDefaultRouteFromTable(std::string table);
    void clearTableMark(std::string table);
    void addTableMark(std::string table, int fwmark);
};

#endif // ROUTINGMANAGER_H
