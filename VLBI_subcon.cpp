/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VLBI_subcon.cpp
 * Author: mschartn
 * 
 * Created on June 29, 2017, 5:51 PM
 */

#include "VLBI_subcon.h"

namespace VieVS{
    VLBI_subcon::VLBI_subcon(): n1scans{0}, n2scans{0} {
    }

    void VLBI_subcon::addScan(const VLBI_scan &scan) noexcept {
        subnet1.push_back(scan);
        n1scans++;
    }

    void
    VLBI_subcon::calcStartTimes(const vector<VLBI_station> &stations, const vector<VLBI_source> &sources) noexcept {

        int i=0;
        while(i<n1scans){
            bool scanValid_slew = true;
            bool scanValid_idle = true;
            vector<unsigned  int> maxIdleTimes;

            int j=0;
            while(j<subnet1[i].getNSta()){
                int staid = subnet1[i].getStationId(j);

                const VLBI_station &thisSta = stations[staid];
                if (thisSta.firstScan()) {
                    subnet1[i].addTimes(j, 0, 0, 0, 0, 0);
                    ++j;
                    maxIdleTimes.push_back(thisSta.getMaxIdleTime());
                    continue;
                }


                thisSta.getCableWrap().calcUnwrappedAz(thisSta.getCurrentPointingVector(),
                                                       subnet1[i].referencePointingVector(j));
                unsigned int slewtime = thisSta.slewTime(subnet1[i].getPointingVector(j));

                if (slewtime > thisSta.getMaxSlewtime()) {
                    scanValid_slew = subnet1[i].removeStation(j);
                    if(!scanValid_slew){
                        break;
                    }
                } else {
                    maxIdleTimes.push_back(thisSta.getMaxIdleTime());
                    subnet1[i].addTimes(j, thisSta.getWaitSetup(), thisSta.getWaitSource(), slewtime,
                                        thisSta.getWaitTape(), thisSta.getWaitCalibration());
                    ++j;
                }
            }

            if (scanValid_slew) {
                scanValid_idle = subnet1[i].checkIdleTimes(maxIdleTimes);
            }

            if (!scanValid_slew || !scanValid_idle){
                subnet1.erase(subnet1.begin()+i);
                --n1scans;
            }else{
                ++i;
            }
        }
    }

    void VLBI_subcon::constructAllBaselines(const vector<VLBI_source> &sources) noexcept {
        int i = 0;
        while (i < n1scans) {
            VLBI_scan &thisScan = subnet1[i];
            const VLBI_source &thisSource = sources[thisScan.getSourceId()];
            bool scanValid = thisScan.constructBaselines(thisSource);
            if (scanValid) {
                ++i;
            } else {
                --n1scans;
                subnet1.erase(subnet1.begin() + i);
            }
        }
    }

    void VLBI_subcon::updateAzEl(const vector<VLBI_station> &stations, const vector<VLBI_source> &sources) noexcept {
        int i = 0;
        while (i < n1scans) {

            const VLBI_source thisSource = sources[subnet1[i].getSourceId()];
            bool scanValid_slew = true;
            bool scanValid_idle = true;
            vector<unsigned  int> maxIdleTimes;

            int j = 0;
            while (j < subnet1[i].getNSta()) {
                int staid = subnet1[i].getPointingVector(j).getStaid();
                VLBI_pointingVector &thisPointingVector = subnet1[i].referencePointingVector(j);
                thisPointingVector.setTime(subnet1[i].getTimes().getEndOfIdleTime(j));
                stations[staid].calcAzEl(thisSource, thisPointingVector);
                bool visible = stations[staid].isVisible(thisPointingVector);
                unsigned int slewtime = numeric_limits<unsigned int>::max();

                if (visible){
                    stations[staid].getCableWrap().calcUnwrappedAz(stations[staid].getCurrentPointingVector(),
                                                                   thisPointingVector);
                    slewtime = stations[staid].slewTime(thisPointingVector);
                }

                if (!visible || slewtime > stations[staid].getMaxSlewtime()){
                    scanValid_slew = subnet1[i].removeStation(j);
                    if(!scanValid_slew){
                        break;
                    }
                } else {
                    maxIdleTimes.push_back(stations[staid].getMaxIdleTime());
                    subnet1[i].updateSlewtime(j, slewtime);
                    ++j;
                }
            }


            if (scanValid_slew) {
                scanValid_idle = subnet1[i].checkIdleTimes(maxIdleTimes);
            }

            if (!scanValid_slew || !scanValid_idle){
                subnet1.erase(subnet1.begin()+i);
                --n1scans;
            }else{
                ++i;
            }
        }
    }

    void
    VLBI_subcon::calcAllBaselineDurations(const vector<VLBI_station> &stations,
                                          const vector<VLBI_source> &sources) noexcept {
        int i = 0;
        while ( i < n1scans ) {
            VLBI_scan& thisScan = subnet1[i];
            bool scanValid = thisScan.calcBaselineScanDuration(stations, sources[thisScan.getSourceId()]);
            if (scanValid){
                ++i;
            } else {
                --n1scans;
                subnet1.erase(subnet1.begin()+i);
            }
        }
    }

    void VLBI_subcon::calcAllScanDurations(const vector<VLBI_station> &stations,
                                           const vector<VLBI_source> &sources) noexcept {
        int i=0;
        while (i<n1scans){
            VLBI_scan& thisScan = subnet1[i];
            int srcid = thisScan.getSourceId();

            bool scanValid = thisScan.scanDuration(stations, sources[srcid]);
            if (scanValid){
                ++i;
            } else {
                --n1scans;
                subnet1.erase(subnet1.begin()+i);
            }
        }
    }

    void VLBI_subcon::createSubcon2(const vector<vector<int>> &subnettingSrcIds, int minStaPerSubcon) noexcept {
        vector<int> sourceIds(n1scans);
        for (int i = 0; i < n1scans; ++i) {
            sourceIds[i] = subnet1[i].getSourceId();
        }

        for (int i = 0; i < n1scans; ++i) {
            int firstSrcId = sourceIds[i];
            VLBI_scan &first = subnet1[i];
            vector<int> secondSrcIds = subnettingSrcIds[firstSrcId];
            for (int j = 0; j < secondSrcIds.size(); ++j) {
                vector<int> sta1 = first.getStationIds();

                int secondSrcId = secondSrcIds[j];
                auto it = find(sourceIds.begin(), sourceIds.end(), secondSrcId);
                if (it != sourceIds.end()) {
                    long idx = distance(sourceIds.begin(), it);
                    VLBI_scan &second = subnet1[idx];
                    vector<int> sta2 = second.getStationIds();


                    vector<int> uniqueSta1;
                    vector<int> uniqueSta2;
                    vector<int> intersection;
                    for (int any: sta1) {
                        if (find(sta2.begin(), sta2.end(), any) == sta2.end()) {
                            uniqueSta1.push_back(any);
                        } else {
                            intersection.push_back(any);
                        }
                    }
                    for (int any: sta2) {
                        if (find(sta1.begin(), sta1.end(), any) == sta1.end()) {
                            uniqueSta2.push_back(any);
                        }
                    }

                    if (uniqueSta1.size() + uniqueSta2.size() + intersection.size() < minStaPerSubcon) {
                        continue;
                    }

                    unsigned long nint = intersection.size();


                    for (int igroup = 0; igroup <= nint; ++igroup) {

                        vector<int> data(nint, 1);
                        for (unsigned long ii = nint - igroup; ii < nint; ++ii) {
                            data.at(ii) = 2;
                        }


                        do {
                            vector<int> scan1sta{uniqueSta1};
                            vector<int> scan2sta{uniqueSta2};
                            for (unsigned long ii = 0; ii < nint; ++ii) {
                                if (data.at(ii) == 1) {
                                    scan1sta.push_back(intersection[ii]);
                                } else {
                                    scan2sta.push_back(intersection[ii]);
                                }
                            }
                            if (scan1sta.size() >= first.getMinimumNumberOfStations() &&
                                scan2sta.size() >= second.getMinimumNumberOfStations()) {

                                unsigned int firstTime = first.maxTime();
                                unsigned int secondTime = second.maxTime();
                                if (firstTime > secondTime) {
                                    swap(firstTime, secondTime);
                                }

                                if (secondTime - firstTime > 600) {
                                    continue;
                                }


                                boost::optional<VLBI_scan> new_first = first.copyScan(scan1sta);
                                if (!new_first) {
                                    continue;
                                }
                                new_first->setType(VLBI_scan::scanType::subnetting);

                                boost::optional<VLBI_scan> new_second = second.copyScan(scan2sta);
                                if (!new_second) {
                                    continue;
                                }
                                new_second->setType(VLBI_scan::scanType::subnetting);

                                ++n2scans;
                                pair<VLBI_scan, VLBI_scan> tmp;
                                tmp.first = move(*new_first);
                                tmp.second = move(*new_second);
                                subnet2.push_back(move(tmp));
                            }
                        } while (next_permutation(std::begin(data), std::end(data)));
                    }
                }
            }
        }
    }

    void VLBI_subcon::generateScore(const vector<VLBI_station> &stations,
                                    const vector<VLBI_skyCoverage> &skyCoverages, unsigned long nsrc) noexcept {

        unsigned long nmaxsta = stations.size();
        vector< vector <double> > firstScore(nsrc);
        for (auto &thisScan: subnet1) {
            vector<double> firstScorePerPv(thisScan.getNSta(),0);
            thisScan.calcScore(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages, firstScorePerPv,stations);
            subnet1_score.push_back(thisScan.getScore());
            firstScore[thisScan.getSourceId()] = std::move(firstScorePerPv);
        }


        for (auto &thisScans:subnet2) {
            VLBI_scan &thisScan1 = thisScans.first;
            int srcid1 = thisScan1.getSourceId();
            thisScan1.calcScore_subcon(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages, firstScore[srcid1],stations);
            double score1 = thisScan1.getScore();

            VLBI_scan &thisScan2 = thisScans.second;
            int srcid2 = thisScan2.getSourceId();
            thisScan2.calcScore_subcon(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages, firstScore[srcid2],stations);
            double score2 = thisScan2.getScore();
            subnet2_score.push_back(score1 + score2);
        }
    }

    void VLBI_subcon::minMaxTime() noexcept {
        unsigned int maxTime_ = 0;
        unsigned int minTime_ = numeric_limits<unsigned int>::max();
        for (auto &thisScan: subnet1) {
            unsigned int thisTime = thisScan.maxTime();
            if (thisTime < minTime_) {
                minTime_ = thisTime;
            }
            if (thisTime > maxTime_) {
                maxTime_ = thisTime;
            }
        }
        for (auto &thisScan: subnet2) {
            unsigned int thisTime1 = thisScan.first.maxTime();
            if (thisTime1 < minTime_) {
                minTime_ = thisTime1;
            }
            if (thisTime1 > maxTime_) {
                maxTime_ = thisTime1;
            }
            unsigned int thisTime2 = thisScan.first.maxTime();
            if (thisTime2 < minTime_) {
                minTime_ = thisTime2;
            }
            if (thisTime2 > maxTime_) {
                maxTime_ = thisTime2;
            }
        }
        minTime = minTime_;
        maxTime = maxTime_;
    }


    void VLBI_subcon::average_station_score(const vector<VLBI_station> &stations) noexcept {

        vector<double> staobs;
        for (auto &thisStation:stations) {
            staobs.push_back(thisStation.getNbls());
        }

        double meanStaobs = std::accumulate(std::begin(staobs), std::end(staobs), 0.0) / staobs.size();

        double total = 0;
        vector<double> staobs_score;
        for (auto &thisStaobs:staobs) {
            double diff = meanStaobs - thisStaobs;
            if (diff > 0) {
                staobs_score.push_back(diff);
                total += diff;
            } else {
                staobs_score.push_back(0);
            }
        }
        if (total != 0) {
            for (auto &thisStaobs_score:staobs_score) {
                thisStaobs_score /= total;
            }
        }
        astas = staobs_score;
    }

    void VLBI_subcon::average_source_score(const vector<VLBI_source> &sources) noexcept {
        vector<double> srcobs;
        for (auto &thisSource:sources) {
            srcobs.push_back(thisSource.getNbls());
        }
        double meanSrcobs = std::accumulate(std::begin(srcobs), std::end(srcobs), 0.0) / srcobs.size();

        double max = 0;
        vector<double> srcobs_score;
        for (auto &thisSrcobs:srcobs) {
            double diff = meanSrcobs - thisSrcobs;
            if (diff > 0) {
                srcobs_score.push_back(diff);
                if (diff > max) {
                    max = diff;
                }
            } else {
                srcobs_score.push_back(0);
            }
        }
        if (max != 0) {
            for (auto &thisStaobs_score:srcobs_score) {
                thisStaobs_score /= max;
            }
        }
        asrcs = srcobs_score;
    }

    void VLBI_subcon::precalcScore(const vector<VLBI_station> &stations, const vector<VLBI_source> &sources) noexcept {

        unsigned long nsta = stations.size();

        nmaxbl = (nsta * (nsta - 1)) / 2;
        if (VLBI_weightFactors::weight_duration != 0) {
            minMaxTime();
        }

        if (VLBI_weightFactors::weight_averageStations != 0) {
            average_station_score(stations);
        }

        if (VLBI_weightFactors::weight_averageSources != 0) {
            average_source_score(sources);
        }

    }

    boost::optional<unsigned long>
    VLBI_subcon::rigorousScore(const vector<VLBI_station> &stations, const vector<VLBI_source> &sources,
                               const vector<VLBI_skyCoverage> &skyCoverages) noexcept {

        vector<double> scores = subnet1_score;
        scores.insert(scores.end(), subnet2_score.begin(), subnet2_score.end());

        std::priority_queue<std::pair<double, int>> q;
        for (unsigned long i = 0; i < scores.size(); ++i) {
            q.push(std::pair<double, unsigned long>(scores[i], i));
        }

        while (true) {
            if (q.size() == 0) {
                return boost::none;
            }
            unsigned long idx = (unsigned long) q.top().second;
            q.pop();

            if (idx < n1scans) {
                unsigned long thisIdx = idx;
                VLBI_scan &thisScan = subnet1[thisIdx];

                bool flag = thisScan.rigorousUpdate(stations, sources[thisScan.getSourceId()]);
                if (!flag) {
                    continue;
                }
                thisScan.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages,stations);
                double newScore = thisScan.getScore();

                q.push(make_pair(newScore, idx));

            } else {
                unsigned long thisIdx = idx - n1scans;
                auto &thisScans = subnet2[thisIdx];

                VLBI_scan &thisScan1 = thisScans.first;
                bool flag1 = thisScan1.rigorousUpdate(stations, sources[thisScan1.getSourceId()]);
                if (!flag1) {
                    continue;
                }
                thisScan1.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages, stations);
                double newScore1 = thisScan1.getScore();

                VLBI_scan &thisScan2 = thisScans.second;
                bool flag2 = thisScan2.rigorousUpdate(stations, sources[thisScan2.getSourceId()]);
                if (!flag2) {
                    continue;
                }

                unsigned int maxTime1 = thisScan1.maxTime();
                unsigned int maxTime2 = thisScan2.maxTime();
                unsigned int deltaTime;
                if (maxTime1 > maxTime2) {
                    deltaTime = maxTime1 - maxTime2;
                } else {
                    deltaTime = maxTime2 - maxTime1;
                }
                if (deltaTime > 600) {
                    continue;
                }

                thisScan2.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages, stations);
                double newScore2 = thisScan2.getScore();
                double newScore = newScore1 + newScore2;

                q.push(make_pair(newScore, idx));
            }
            int newIdx = q.top().second;
            if (newIdx == idx) {
                return idx;
            }
        }
    }

    void VLBI_subcon::removeScan(unsigned long idx) noexcept {
        if (idx < n1scans) {
            unsigned long thisIdx = idx;
            subnet1.erase(subnet1.begin() + thisIdx);
            --n1scans;
            subnet1_score.erase(subnet1_score.begin() + thisIdx);

        } else {
            unsigned long thisIdx = idx - n1scans;
            subnet2.erase(subnet2.begin() + thisIdx);
            --n2scans;
            subnet2_score.erase(subnet2_score.begin() + thisIdx);

        }
    }

    void VLBI_subcon::clearSubnettingScans() {
        n2scans = 0;
        subnet2.clear();
        subnet2_score.clear();
    }


}
