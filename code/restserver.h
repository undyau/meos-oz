#pragma once

/************************************************************************
MeOS - Orienteering Software
Copyright (C) 2009-2017 Melin Software HB

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

/** Class for providing a MeOS REST service */

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <condition_variable>

namespace restbed {
  class Service;
  class Session;
};

class RestServer {
private:
  struct EventRequest {
    EventRequest() : state(false) {}
    multimap<string, string> parameters;
    string answer;
    atomic_bool state; //false - asked, true - answerd

    bool isCompleted() { 
      return state; 
    }
  };

  mutex lock;
  atomic_bool hasAnyRequest;
  shared_ptr<thread> service;
  shared_ptr<restbed::Service> restService;
  condition_variable waitForCompletion;

  deque<shared_ptr<EventRequest>> requests;
  void getData(oEvent &ref, const string &what, const multimap<string, string> &param, string &answer);
  void compute(oEvent &ref);
  void startThread(int port);

  static void getSelection(const string &param, set<int> &sel);

  void handleRequest(const shared_ptr<restbed::Session> &session);
  friend void method_handler(const shared_ptr< restbed::Session > session);

  shared_ptr<EventRequest> addRequest(multimap<string, string> &param);
  shared_ptr<EventRequest> getRequest();

  vector<int> responseTimes;
  
  RestServer();
  static vector< shared_ptr<RestServer> > startedServers;
 
  RestServer(const RestServer &);
  RestServer & operator=(const RestServer &) const;


  void computeInternal(oEvent &ref, shared_ptr<RestServer::EventRequest> &rq);

  map<int, pair<oListParam, shared_ptr<oListInfo> > > listCache;
public:

  ~RestServer();

  void startService(int port);
  void stop();
  
  static shared_ptr<RestServer> construct();
  static void remove(shared_ptr<RestServer> server);

  static void computeRequested(oEvent &ref);

  struct Statistics {
    int numRequests;
    int averageResponseTime;
    int maxResponseTime;
  };

  void getStatistics(Statistics &s);
};


