//
// Created by matth on 28.01.2018.
//
#ifndef VIEVS_SCHEDULER_H
#define VIEVS_SCHEDULER_H

#include <cstdlib>
#include <chrono>
#include <vector>
#include <boost/format.hpp>
#include <iostream>


#include "Initializer.h"
#include "Scheduler.h"
#include "Output.h"
#include "ParameterSettings.h"
#include "HighImpactScanDescriptor.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace VieVS {

    class VieSchedpp {
    public:
        VieSchedpp() = default;

        explicit VieSchedpp(const std::string &inputFile);

        void run();



    private:
        std::string inputFile_;
        std::string path_;
        std::string fileName_;
        boost::property_tree::ptree xml_; ///< content of parameters.xml file

        int nThreads_ {1};
        std::string jobScheduling_ {"auto"};
        int chunkSize_ {1};
        std::string threadPlace_ {"auto"};

        SkdCatalogReader skdCatalogs_;
        std::vector<VieVS::MultiScheduling::Parameters> multiSchedParameters_;

        void readSkdCatalogs();

        void readMultiCoreSetup();

    };
}

#endif //VIEVS_SCHEDULER_H
