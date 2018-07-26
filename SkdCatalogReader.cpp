//
// Created by matth on 17.09.2017.
//

#include "SkdCatalogReader.h"

using namespace VieVS;
using namespace std;
unsigned long SkdCatalogReader::nextId = 0;

// source.cat: ^[^\*]\s*([\w+\$-]*)\s+([\w+\$-]*)\s+(\d{1,2})\s+(\d{1,2})\s+(\d{0,2}\.\d*)\s+([+-]?[\d]*)\s+(\d{1,2})\s+(\d{0,2}\.\d*)
// antenna.cat: ^[^*]\s*(\w)\s+([\w-]*)\s*(\w*)\s+(\d*\.?\d*)\s+(\d*\.?\d*)\s+(\d*)\s+([+|-]?\d*\.?\d*)\s+([+|-]?\d*\.?\d*)\s+(\d*\.?\d*)\s+(\d*)\s+([+|-]?\d*\.?\d*)\s+([+|-]?\d*\.?\d*)\s+(\d*\.?\d*)\s+(\w+)\s+(\w+)\s+([\w-]+)


SkdCatalogReader::SkdCatalogReader():VieVS_Object(nextId++),bandWidth_{0},sampleRate_{0},bits_{0} {
}


//TODO only save catalogs of required stations!
std::map<std::string, std::vector<std::string>>
SkdCatalogReader::readCatalog(SkdCatalogReader::CATALOG type) noexcept {

    map<string, vector<string>> all;
    int indexOfKey;
    string filepath;

    // switch between four available catalogs
    switch (type) {
        case CATALOG::antenna: {
            filepath = antennaPath_;
            indexOfKey = 1;
            break;
        }
        case CATALOG::position: {
            filepath = positionPath_;
            indexOfKey = 0;
            break;
        }
        case CATALOG::equip: {
            filepath = equipPath_;
            indexOfKey = 1;
            break;
        }
        case CATALOG::mask: {
            filepath = maskPath_;
            indexOfKey = 2;
            break;
        }
        case CATALOG::source: {
            filepath = sourcePath_;
            indexOfKey = 0;
            break;
        }
        case CATALOG::flux: {
            filepath = fluxPath_;
            indexOfKey = 0;
            break;
        }
    }

    bool fromSkdFile = false;
    string skdFlag;
    if(filepath.length() > 4 && filepath.substr(filepath.length()-4) == ".skd") {
        fromSkdFile = true;
        switch (type) {
            case CATALOG::antenna:
            case CATALOG::position:
            case CATALOG::equip:
            case CATALOG::mask: {skdFlag = "$STATIONS"; break; }
            case CATALOG::source:{skdFlag = "$SOURCES"; break; }
            case CATALOG::flux:{skdFlag = "$FLUX"; break; }
        }
    }

    // read in CATALOG. antenna, position and equip use the same routine
    switch (type) {
        case CATALOG::antenna:
        case CATALOG::position:
        case CATALOG::equip:
        case CATALOG::source: {

            // open file
            ifstream fid(filepath);
            if (!fid.is_open()) {
                #ifdef VIESCHEDPP_LOG
                BOOST_LOG_TRIVIAL(error) << "unable to open " << filepath << " file";
                #else
                cout << "unable to open " << filepath << " file";
                #endif
            } else {
                string line;
                // if read from skd file read until you reach flag
                if(fromSkdFile){
                    while(getline(fid,line)){
                        if(boost::trim_copy(line) == skdFlag){
                            break;
                        }
                    }
                }
                bool versionFound = false;
                std::map<string,string> eqId2staName;
                // loop through file
                while (getline(fid, line)) {
                    if(!versionFound && line.length() > 0){
                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                        if(splitVector.size()>=3){
                            if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                                if(type == CATALOG::antenna){
                                    catalogsVersion_["antenna"] = splitVector.at(2);
                                    versionFound = true;
                                }
                                if(type == CATALOG::position){
                                    catalogsVersion_["position"] = splitVector.at(2);
                                    versionFound = true;
                                }
                                if(type == CATALOG::equip) {
                                    catalogsVersion_["equip"] = splitVector.at(2);
                                    versionFound = true;
                                }
                                if(type == CATALOG::source){
                                    catalogsVersion_["source"] = splitVector.at(2);
                                    versionFound = true;
                                }
                            }
                        }
                    }

                    if (line.length() > 0 && line.at(0) != '*') {
                        // trim leading and trailing blanks
                        line = boost::algorithm::trim_copy(line);
                        if(line.at(0) == '$'){
                            break;
                        }

                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

                        if(fromSkdFile && type == CATALOG::antenna && line.at(0)!='A'){
                            continue;
                        }
                        if(fromSkdFile && type == CATALOG::position && line.at(0)!='P'){
                            continue;
                        }
                        if(fromSkdFile && type == CATALOG::equip && line.at(0)!='T'){
                            if( line.at(0) == 'A'){
                                eqId2staName[boost::algorithm::to_upper_copy(splitVector[15])] = splitVector[2];
                            }
                            continue;
                        }
                        if(fromSkdFile && type != CATALOG::source){
                            splitVector.erase(splitVector.begin());
                        }
                        if(fromSkdFile && type == CATALOG::equip){
                            splitVector.insert(splitVector.begin(),eqId2staName[boost::algorithm::to_upper_copy(splitVector[indexOfKey-1])]);
                        }


                        // get key and convert it to upper case for case insensitivity
                        string key = boost::algorithm::to_upper_copy(splitVector[indexOfKey]);
                        // add station name to key if you look at equip.cat because id alone is not unique in catalogs
                        if (type == CATALOG::equip) {
                            key = boost::algorithm::to_upper_copy(key + "|" + splitVector[indexOfKey - 1]);
                        }

                        // save all station keys in case of antenna catalog
                        if (type == CATALOG::antenna){
                            // look if this station is really required
                            if(find(staNames_.begin(),staNames_.end(),key) == staNames_.end()){
                                continue;
                            }
                            antennaKey2positionKey_[key] = boost::algorithm::to_upper_copy(splitVector.at(13));
                            string id_EQ = boost::algorithm::to_upper_copy(splitVector.at(14)+"|"+key);
                            antennaKey2equipKey_[key] = id_EQ;
                            antennaKey2maskKey_[key] = boost::algorithm::to_upper_copy(splitVector.at(15));
                        } else if (type == CATALOG::position){
                            if(!util::valueExists(antennaKey2positionKey_,key)){
                                continue;
                            }
                        } else if (type == CATALOG::equip){
                            if(!util::valueExists(antennaKey2equipKey_,key)){
                                continue;
                            }
                        } else if (type == CATALOG::mask){
                            if(!util::valueExists(antennaKey2maskKey_,key)){
                                continue;
                            }
                        }


                        // look if a key already exists, if not add it.
                        if (all.find(key) == all.end()) {
                            all.insert(pair<string, vector<string>>(key, splitVector));
                        } else {
                            #ifdef VIESCHEDPP_LOG
                            BOOST_LOG_TRIVIAL(warning) << "Duplicated element of '" << key << "' in " << filepath << " -> ignored";
                            #else
                            cout << "Duplicated element of '" << key << "' in " << filepath << " -> ignored";
                            #endif
                        }
                    }
                }
            }
            // close file
            fid.close();
            break;
        }

        case CATALOG::mask: {

            // open file
            ifstream fid(filepath);
            if (!fid.is_open()) {
                #ifdef VIESCHEDPP_LOG
                BOOST_LOG_TRIVIAL(error) << "unable to open " << filepath;
                #else
                cout << "unable to open " << filepath;
                #endif
            } else {
                string line;
                // if read from skd file read until you reach flag
                if(fromSkdFile){
                    while(getline(fid,line)){
                        if(boost::trim_copy(line) == skdFlag){
                            break;
                        }
                    }
                }
                vector<string> splitVector_total;

                bool versionFound = false;
                // loop through CATALOG
                while (getline(fid, line)) {
                    if(!versionFound && line.length() > 0){
                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                        if(splitVector.size()>=3){
                            if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                                catalogsVersion_["mask"] = splitVector.at(2);
                                versionFound = true;
                            }
                        }
                    }


                    if (line.length() > 0 && line.at(0) != '*') {
                        // trim leading and trailing blanks
                        line = boost::algorithm::trim_copy(line);
                        if(line.at(0) == '$'){
                            break;
                        }
                        if(fromSkdFile && line.at(0) != 'H'){
                            continue;
                        }

                        // if first element is not an '-' this line belongs to new station mask
                        if (line.at(0) != '-' && !splitVector_total.empty()) {


                            if(fromSkdFile){
                                splitVector_total.insert(splitVector_total.begin()+1," ");
                            }

                            // get key and convert it to upper case for case insensitivity
                            string key = boost::algorithm::to_upper_copy(splitVector_total[indexOfKey]);

                            // previous mask is finished, add it to map
                            all.insert(pair<string, vector<string>>(key, splitVector_total));
                            splitVector_total.clear();
                        }

                        // split vector
                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

                        // if it is a new mask add all elements to vector, if not start at the 2nd element (ignore '-')
                        if (splitVector_total.empty()) {
                            splitVector_total.insert(splitVector_total.end(), splitVector.begin(), splitVector.end());
                        } else {
                            splitVector_total.insert(splitVector_total.end(), splitVector.begin() + 1,
                                                     splitVector.end());
                        }
                    }
                }


                if(fromSkdFile){
                    splitVector_total.insert(splitVector_total.begin()+1," ");
                }
                string key = boost::algorithm::to_upper_copy(splitVector_total[indexOfKey]);

                all.insert(pair<string, vector<string>>(key, splitVector_total));

            }
            break;
        }

        case CATALOG::flux: {
            // open file
            ifstream fid(filepath);
            if (!fid.is_open()) {
                #ifdef VIESCHEDPP_LOG
                BOOST_LOG_TRIVIAL(error) << "unable to open " << filepath;
                #else
                cout << "unable to open " << filepath;
                #endif
            } else {
                string line;
                vector<string> lines;
                // if read from skd file read until you reach flag
                if(fromSkdFile){
                    while(getline(fid,line)){
                        if(boost::trim_copy(line) == skdFlag){
                            break;
                        }
                    }
                }
                vector<string> splitVector_total;
                string sourceName;

                bool versionFound = false;
                // get first entry
                while (getline(fid, line)) {
                    line = boost::algorithm::trim_copy(line);
                    if(!versionFound && line.length() > 0){
                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                        if(splitVector.size()>=3){
                            if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                                catalogsVersion_["flux"] = splitVector.at(2);
                                versionFound = true;
                            }
                        }
                    }


                    if (line.length() > 0 && line.at(0) != '*') {
                        boost::split(splitVector_total, line, boost::is_space(), boost::token_compress_on);
                        sourceName = splitVector_total[indexOfKey];
                        lines.push_back(line);
                        break;
                    }
                }

                // loop through CATALOG
                while (getline(fid, line)) {
                    // trim leading and trailing blanks
                    line = boost::algorithm::trim_copy(line);
                    if (line.length() > 0 && line.at(0) != '*') {
                        if(line.at(0) == '$'){
                            break;
                        }

                        vector<string> splitVector;
                        boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                        string newStation = splitVector[indexOfKey];

                        if (newStation == sourceName) {
                            lines.push_back(line);
                            splitVector_total.insert(splitVector_total.end(), splitVector.begin(), splitVector.end());
                        } else {
                            all.insert(pair<string, vector<string>>(sourceName, lines));
                            lines.clear();
                            lines.push_back(line);
                            sourceName = newStation;
                            splitVector_total = splitVector;
                        }

                    }
                }
                all.insert(pair<string, vector<string>>(sourceName, lines));
            }
            break;
        }
    }

    return all;
}

void SkdCatalogReader::setCatalogFilePathes(const boost::property_tree::ptree &ptreeWithPathes) {
    sourcePath_ = ptreeWithPathes.get<string>("source");
    fluxPath_ = ptreeWithPathes.get<string>("flux");

    antennaPath_ = ptreeWithPathes.get<string>("antenna");
    positionPath_ = ptreeWithPathes.get<string>("position");
    equipPath_ = ptreeWithPathes.get<string>("equip");
    maskPath_ = ptreeWithPathes.get<string>("mask");

    modesPath_ = ptreeWithPathes.get<string>("modes");
    recPath_ = ptreeWithPathes.get<string>("rec");
    tracksPath_ = ptreeWithPathes.get<string>("tracks");
    freqPath_ = ptreeWithPathes.get<string>("freq");
    rxPath_ = ptreeWithPathes.get<string>("rx");
    loifPath_ = ptreeWithPathes.get<string>("loif");
}


void SkdCatalogReader::setCatalogFilePathes(const std::string &antenna, const std::string &equip, const std::string &flux,
                          const std::string &freq, const std::string &hdpos, const std::string &loif,
                          const std::string &mask, const std::string &modes, const std::string &position,
                          const std::string &rec, const std::string &rx, const std::string &source,
                          const std::string &tracks){

    sourcePath_ = source;
    fluxPath_ = flux;

    antennaPath_ = antenna;
    positionPath_ = position;
    equipPath_ = equip;
    maskPath_ = mask;

    modesPath_ = modes;
    recPath_ = rec;
    tracksPath_ = tracks;
    freqPath_ = freq;
    rxPath_ = rx;
    loifPath_ = loif;

}
void SkdCatalogReader::setCatalogFilePathes(const std::string &skdFile){

    sourcePath_ = skdFile;
    fluxPath_ = skdFile;

    antennaPath_ = skdFile;
    positionPath_ = skdFile;
    equipPath_ = skdFile;
    maskPath_ = skdFile;

    modesPath_ = "";
    recPath_ = "";
    tracksPath_ = "";
    freqPath_ = "";
    rxPath_ = "";
    loifPath_ = "";

}


void SkdCatalogReader::initializeSourceCatalogs() {
    sourceCatalog_ = readCatalog(CATALOG::source);
    fluxCatalog_ = readCatalog(CATALOG::flux);
}

void SkdCatalogReader::initializeStationCatalogs() {
    antennaCatalog_ = readCatalog(CATALOG::antenna);
    positionCatalog_ = readCatalog(CATALOG::position);
    equipCatalog_ = readCatalog(CATALOG::equip);
    maskCatalog_ = readCatalog(CATALOG::mask);

    saveOneLetterCode();
    saveTwoLetterCode();
}

void SkdCatalogReader::initializeModesCatalogs(const string &obsModeName) {
    readModesCatalog(obsModeName);
    readRecCatalog();
    readTracksCatalog();
    readFreqCatalog();
    readRxCatalog();
    readLoifCatalog();
}

void SkdCatalogReader::readModesCatalog(const string &obsModeName) {
    ifstream fmodes(modesPath_);
    string line;
    bool versionFound = false;
    while (getline(fmodes, line)) {
        if(!versionFound && line.length() > 0){
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
            if(splitVector.size()>=3){
                if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                    catalogsVersion_["modes"] = splitVector.at(2);
                    versionFound = true;
                }
            }
        }


        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
            line = boost::algorithm::trim_copy(line);
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

            if (splitVector[0] == obsModeName) {
                freqName_ = splitVector[1];
                bandWidth_ = boost::lexical_cast<double>(splitVector[2]);
                sampleRate_ = boost::lexical_cast<double>(splitVector[3]);
                recName_ = splitVector[4];
                break;
            }

        }
    }
    fmodes.close();
}

void SkdCatalogReader::readRecCatalog() {
    ifstream frec(recPath_);
    string line;
    bool versionFound = false;
    while (getline(frec, line)) {
        if(!versionFound && line.length() > 0){
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
            if(splitVector.size()>=3){
                if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                    catalogsVersion_["rec"] = splitVector.at(2);
                    versionFound = true;
                }
            }
        }

        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
            line = boost::algorithm::trim_copy(line);
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

            if (splitVector[0] == recName_) {
                while (getline(frec, line)) {
                    if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                        line = boost::algorithm::trim_copy(line);
                        if (line[0] != '-') {
                            break;
                        }

                        vector<string> splitVector2;
                        boost::split(splitVector2, line, boost::is_space(), boost::token_compress_on);

                        string thisStaName = splitVector2[1];
                        if (find(staNames_.begin(), staNames_.end(), thisStaName) != staNames_.end()) {
                            staName2hdposMap_[thisStaName] = splitVector2[2];
                            staName2tracksMap_[thisStaName] = splitVector2[3];
                            if (find(tracksIds_.begin(), tracksIds_.end(), splitVector2[3]) == tracksIds_.end()) {
                                tracksIds_.push_back(splitVector2[3]);
                            }

                            staName2recFormatMap_[thisStaName] = splitVector2[4];
                            if (splitVector2.size() > 5) {
                                #ifdef VIESCHEDPP_LOG
                                BOOST_LOG_TRIVIAL(warning) << "barrel_roll and max_bw information ignored for station " << thisStaName << " in rec.cat";
                                #else
                                cout << "barrel_roll and max_bw information ignored for station " << thisStaName << " in rec.cat";
                                #endif
                            }
                        }
                    }
                }
            }
        }
    }
    frec.close();
}

void SkdCatalogReader::readTracksCatalog() {
    for (const auto &tracksId:tracksIds_) {
        ifstream ftracks(tracksPath_);
        string line;

        bool versionFound = false;
        while (getline(ftracks, line)) {
            if(!versionFound && line.length() > 0){
                vector<string> splitVector;
                boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                if(splitVector.size()>=3){
                    if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                        catalogsVersion_["tracks"] = splitVector.at(2);
                        versionFound = true;
                    }
                }
            }

            if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                line = boost::algorithm::trim_copy(line);
                vector<string> splitVector;
                boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

                if (splitVector[0] == tracksId) {
                    tracksId2fanoutMap_[tracksId] = boost::lexical_cast<int>(splitVector[1]);
                    auto bits = boost::lexical_cast<unsigned int>(splitVector[2]);
                    if(bits_ == 0){
                        bits_ = bits;
                    } else if(bits_ != bits){
                        #ifdef VIESCHEDPP_LOG
                        BOOST_LOG_TRIVIAL(error) << "number of recorded bits is different for different track ids -> ignored";
                        #else
                        cout << "number of recorded bits is different for different track ids -> ignored";
                        #endif
                    }
                    tracksId2bitsMap_[tracksId] = bits;

                    while (getline(ftracks, line)) {
                        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                            line = boost::algorithm::trim_copy(line);
                            if (line[0] != '-') {
                                break;
                            }

                            vector<string> splitVector2;
                            boost::split(splitVector2, line, boost::is_space(), boost::token_compress_on);
                            auto channelNumber = boost::lexical_cast<int>(splitVector2[1]);
                            tracksId2channelNumber2tracksMap_[tracksId][channelNumber] = splitVector2[2];
                        }
                    }
                }
            }
        }
        ftracks.close();
    }
}

void SkdCatalogReader::readFreqCatalog() {
    ifstream ffreq(freqPath_);
    string line;

    bool versionFound = false;
    while (getline(ffreq, line)) {
        if(!versionFound && line.length() > 0){
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
            if(splitVector.size()>=3){
                if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                    catalogsVersion_["freq"] = splitVector.at(2);
                    versionFound = true;
                }
            }
        }

        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
            line = boost::algorithm::trim_copy(line);
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

            if (splitVector[0] == freqName_) {
                freqTwoLetterCode_ = splitVector[1];
                rxName_ = splitVector[3];

                while (getline(ffreq, line)) {
                    if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                        line = boost::algorithm::trim_copy(line);
                        if (line[0] != '-') {
                            break;
                        }

                        vector<string> splitVector2;
                        boost::split(splitVector2, line, boost::is_space(), boost::token_compress_on);

                        string channelNumberStr = splitVector2[5];
                        auto channelNumber = boost::lexical_cast<int>(
                                channelNumberStr.substr(2, channelNumberStr.size() - 2));

                        channelNumber2band_[channelNumber] = splitVector2[1];
                        channelNumber2polarization_[channelNumber] = splitVector2[2];
                        channelNumber2skyFreq_[channelNumber] = splitVector2[3];
                        channelNumber2sideBand_[channelNumber] = splitVector2[4];
                        channelNumber2BBC_[channelNumber] = splitVector2[6];
                        channelNumber2phaseCalFrequency_[channelNumber] = splitVector2[7];
                    }
                }
            }
        }
    }
    ffreq.close();
}

void SkdCatalogReader::readRxCatalog() {
    ifstream frx(rxPath_);
    string line;

    bool versionFound = false;
    while (getline(frx, line)) {
        if(!versionFound && line.length() > 0){
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
            if(splitVector.size()>=3){
                if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                    catalogsVersion_["rx"] = splitVector.at(2);
                    versionFound = true;
                }
            }
        }

        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
            line = boost::algorithm::trim_copy(line);
            vector<string> splitVector;
            boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

            if (splitVector[0] == rxName_) {
                while (getline(frx, line)) {
                    if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                        line = boost::algorithm::trim_copy(line);
                        if (line[0] != '-') {
                            break;
                        }

                        vector<string> splitVector2;
                        boost::split(splitVector2, line, boost::is_space(), boost::token_compress_on);


                        string thisStaName = splitVector2[1];
                        if (find(staNames_.begin(), staNames_.end(), thisStaName) != staNames_.end()) {

                            if (find(loifIds_.begin(), loifIds_.end(), splitVector2[2]) == loifIds_.end()) {
                                loifIds_.push_back(splitVector2[2]);
                            }

                            staName2loifId_[thisStaName] = splitVector2[2];
                        }
                    }
                }
            }
        }
    }
    frx.close();
}

void SkdCatalogReader::readLoifCatalog() {
    string line;
    for (const auto &loifId:loifIds_) {
        ifstream floif(loifPath_);

        bool versionFound = false;
        while (getline(floif, line)) {
            if(!versionFound && line.length() > 0){
                vector<string> splitVector;
                boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);
                if(splitVector.size()>=3){
                    if(boost::to_lower_copy(splitVector.at(1)) == "version"){
                        catalogsVersion_["loif"] = splitVector.at(2);
                        versionFound = true;
                    }
                }
            }

            if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                line = boost::algorithm::trim_copy(line);
                vector<string> splitVector;
                boost::split(splitVector, line, boost::is_space(), boost::token_compress_on);

                if (splitVector[0] == loifId) {
                    vector<string> loifInfo;

                    while (getline(floif, line)) {
                        if (line.length() > 0 && (line.at(0) != '*' && line.at(0) != '&' && line.at(0) != '!')) {
                            line = boost::algorithm::trim_copy(line);
                            if (line[0] != '-') {
                                loifId2loifInfo_[loifId] = loifInfo;
                                break;
                            }
                            loifInfo.push_back(line);
                        }
                    }
                }
            }
        }
        floif.close();
    }

}

void SkdCatalogReader::saveOneLetterCode() {

    const map<string, vector<string> > ant = antennaCatalog_;

    std::set<char> charsUsed;

    for (const auto &staName:staNames_) {
        auto tmp = ant.at(staName);
        char oneLetterCode = tmp[0][0];
        if (charsUsed.find(oneLetterCode) != charsUsed.end()) {

            for (char l = 'A'; l <= 'Z'; ++l) {
                if (charsUsed.find(l) == charsUsed.end()) {
                    oneLetterCode = l;
                    break;
                }
            }

            #ifdef VIESCHEDPP_LOG
            BOOST_LOG_TRIVIAL(warning) << "changing one letter code of station " << staName << " to '"<< oneLetterCode <<"'";
            #else
            cout << "changing one letter code of station " << staName << " to '"<< oneLetterCode <<"'";
            #endif

        }
        charsUsed.insert(oneLetterCode);
        oneLetterCode_[staName] = oneLetterCode;
    }
}

void SkdCatalogReader::saveTwoLetterCode() {

    const map<string, vector<string> > ant = antennaCatalog_;

    map<string, string> twoLetterCodeMap;
    for (const auto &staName:staNames_) {
        auto tmp = ant.at(staName);
        string twoLetterCode = tmp[13];
        twoLetterCode_[staName] = twoLetterCode;
    }

}

std::string SkdCatalogReader::getVersion(const std::string& name) const {
    if (catalogsVersion_.find(name) != catalogsVersion_.end()){
        return catalogsVersion_.at(name);
    }else{
        return "Unknown";
    }
}





