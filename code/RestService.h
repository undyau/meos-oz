/************************************************************************
MeOS - Orienteering Software
Copyright (C) 2009-2018 Melin Software HB

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Melin Software HB - software@melin.nu - www.melin.nu
Eksoppsv�gen 16, SE-75646 UPPSALA, Sweden

************************************************************************/

#pragma once
#include "TabAuto.h"
#include <memory>

class RestServer;

class RestService :
  public AutoMachine, GuiHandler
{
  int port;
  shared_ptr<RestServer> server;

public:
  
  void save(oEvent &oe, gdioutput &gdi) override;
  void settings(gdioutput &gdi, oEvent &oe, bool created) override;
  RestService *clone() const override { return new RestService(*this); }
  void status(gdioutput &gdi) override;
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast) override;
  
  void handle(gdioutput &gdi, BaseInfo &info, GuiEventType type) override;
  
  RestService();
  ~RestService();
};

