/**
 * @file Subcon.h
 * @brief class Subcon
 *
 *
 * @author Matthias Schartner
 * @date 29.06.2017
 */

#ifndef SUBCON_H
#define SUBCON_H
#include <vector>
#include <utility>
#include <limits>
#include <queue>
#include <boost/optional.hpp>
#include <numeric>

#include "Network.h"
#include "Source.h"
#include "Scan.h"
#include "StationEndposition.h"
#include "Subnetting.h"


namespace VieVS{
    /**
     * @class Subcon
     * @brief representation of a VLBI subcon
     *
     * @author Matthias Schartner
     * @date 29.06.2017
     */
    class Subcon: public VieVS_Object {
    public:

        /**
         * @brief empty default constructor
         */
        Subcon();

        /**
         * @brief add a single source scan to subcon
         *
         * @param scan scan which should be added
         */
        void addScan(Scan &&scan) noexcept;

        /**
         * @brief removes a scan from the subcon
         *
         * The index counts first through all single source scans and continues with all subnetting scans. If the index
         * is larger than the number of single scans both subnetting scans will be removed.
         *
         * @param idx index of scan which should be removed
         */
        void removeScan(unsigned long idx) noexcept;

        /**
         * @brief getter for number of possible single source scans
         *
         * @return number of possible single source scans
         */
        unsigned long getNumberSingleScans() const noexcept {
            return nSingleScans_;
        }

        /**
         * @brief getter for number of possible subnetting scans
         *
         * @return number of possible subnetting scans
         */
        unsigned long getNumberSubnettingScans() const noexcept {
            return nSubnettingScans_;
        }

        /**
         * @brief getter for a single source scan
         *
         * @param idx index
         * @return single source scan at this index
         */
        Scan takeSingleSourceScan(unsigned long idx) noexcept {
            Scan tmp = std::move(singleScans_[idx]);
            singleScans_.erase(singleScans_.begin()+idx);
            --nSingleScans_;
            return std::move(tmp);
        }

        /**
         * @brief getter for subnettin scan
         *
         * @param idx index
         * @return subnetting scan at this index
         */
        std::pair<Scan, Scan> takeSubnettingScans(unsigned long idx) noexcept {
            std::pair<Scan, Scan> tmp = std::move(subnettingScans_[idx]);
            subnettingScans_.erase(subnettingScans_.begin()+idx);
            --nSubnettingScans_;
            return std::move(tmp);
        }

        /**
         * @brief calculates the earliest possible start time for all single source scans in this subcon
         *
         * @param stations list of all stations
         * @param sources list of all sources
         */
        void calcStartTimes(const Network &network, const std::vector<Source> &sources,
                            const boost::optional<StationEndposition> &endposition = boost::none) noexcept;

        /**
         * @brief constructs all baselines for all single source scans in this subcon
         *
         * @param sources list of all sources
         */
        void constructAllBaselines(const Network &network, const std::vector<Source> &sources) noexcept;

        /**
         * @brief updates all azimuths and elevations of all pointing vectors for each single source scan in this subcon
         *
         * @param stations list of all stations
         * @param sources list of all sources
         */
        void updateAzEl(const Network &network, const std::vector<Source> &sources) noexcept;

        /**
         * @brief calculates all baseline scan duration for all single source scans in this subcon
         *
         * @param stations list of all stations
         * @param sources list of all sources
         */
        void
        calcAllBaselineDurations(const Network &network, const std::vector<Source> &sources) noexcept;

        /**
         * @brief calculates all scan duration of all single source scans in this subcon
         *
         * @param stations list of all stations
         * @param sources list of all sources
         */
        void calcAllScanDurations(const Network &network, const std::vector<Source> &sources,
                                  const boost::optional<StationEndposition> &endposition = boost::none) noexcept;

        /**
         * @brief create all subnetting scans from possible single source scans
         *
         * @param subnettingSrcIds ids between all sources which could be used for subnetting
         * @param minStaPerSubcon  minimum number of stations per subconfiguration
         */
        void createSubnettingScans(const Subnetting &subnetting, const std::vector<Source> &sources) noexcept;

        /**
         * @brief generate scores for all single source and subnetting scans
         *
         * @param stations list of all stations
         * @param skyCoverages list of all sky coverages
         */
        void generateScore(const Network &network, const std::vector<Source> &sources) noexcept;

        void generateScore(const std::vector<double> &lowElevatrionScore, const std::vector<double> &highElevationScore,
                           const Network &network, const std::vector<Source> &sources);

        void generateScore(const Network &network, const std::vector<Source> &sources,
                           const std::vector<std::map<unsigned long,double>> &hiscores, unsigned int interval);

        void checkIfEnoughTimeToReachEndposition(const Network & network,
                                                 const std::vector<Source> &sources,
                                                 const boost::optional<StationEndposition> &endposition = boost::none);

        /**
         * @brief get minimum and maximum time required for a possible scan
         */
        void minMaxTime() noexcept;



//        std::vector<Scan> selectBest(const Network &network, const std::vector<Source> &sources);
        /**
         * @brief rigorousely updates the best scans untill the best one is found
         *
         * @param stations list of all stations
         * @param sources list of all sources
         * @param skyCoverages list of all sky coverages
         * @param prevLowElevationScores optinal argument if you have a calibrator block scan - previouse low elevation scores
         * @param prevHighElevationScores optinal argument if you have a calibrator block scan - previouse high elevation scores
         * @return index of best scan
         */
        std::vector<Scan> selectBest(const Network &network, const std::vector<Source> &sources,
                                     const boost::optional<StationEndposition> &endposition = boost::none) noexcept;

        std::vector<Scan> selectBest(const Network &network, const std::vector<Source> &sources,
                                     const std::vector<double> &prevLowElevationScores,
                                     const std::vector<double> &prevHighElevationScores,
                                     const boost::optional<StationEndposition> &endposition = boost::none) noexcept;


        /**
         * @brief clear all subnetting scans
         *
         * Usually unused
         */
        void clearSubnettingScans();


        boost::optional<unsigned long> rigorousScore(const Network &network, const std::vector<Source> &sources,
                                                     const std::vector<double> &prevLowElevationScores,
                                                     const std::vector<double> &prevHighElevationScores);

        void calcCalibratorScanDuration(const std::vector<Station> &stations, const std::vector<Source> &sources);


        void changeType(Scan::ScanType type);

        void visibleScan(unsigned int currentTime, Scan::ScanType type, const Network &network,
                         const Source &thisSource, std::set<unsigned long>observedSources = std::set<unsigned long>());

    private:
        static unsigned long nextId;

        unsigned long nSingleScans_; ///< number of single source scans
        std::vector<Scan> singleScans_; ///< all single source scans

        unsigned long nSubnettingScans_; ///< number of subnetting scans
        std::vector<std::pair<Scan, Scan> > subnettingScans_; ///< all subnetting scans

        unsigned int minRequiredTime_; ///< minimum time required for a scan
        unsigned int maxRequiredTime_; ///< maximum time required for a scan
        std::vector<double> astas_; ///< average station score for each station
        std::vector<double> asrcs_; ///< average source score for each source
        std::vector<double> abls_; ///< average baseline score for each baseline


        /**
         * @brief precalculate all necessary parameters to generate scores
         *
         * @param stations list of all stations
         * @param sources list of all sources
         */
        void precalcScore(const Network &network, const std::vector<Source> &sources) noexcept;
        /**
         * @brief calculate the score for averaging out each station
         *
         * @param stations list of all stations
         */
        void prepareAverageScore(const std::vector<Station> &sources) noexcept;


        void prepareAverageScore(const std::vector<Baseline> &sources) noexcept;

        /**
         * @brief calculate the score for averaging out each source
         * @param sources list of all sources
         */
        void prepareAverageScore(const std::vector<Source> &sources) noexcept;

        std::vector<double> prepareAverageScore_base(const std::vector<unsigned long> &nobs) noexcept;
    };
}
#endif /* SUBCON_H */

