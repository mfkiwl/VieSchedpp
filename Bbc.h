#include <utility>

/*
 *  VieSched++ Very Long Baseline Interferometry (VLBI) Scheduling Software
 *  Copyright (C) 2018  Matthias Schartner
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
* @file Bbc.h
* @brief class Bbc
*
* @author Matthias Schartner
* @date 17.09.2018
*/

#ifndef VIESCHEDPP_BBC_H
#define VIESCHEDPP_BBC_H

#include <utility>
#include <vector>
#include <fstream>
#include <boost/format.hpp>
#include "VieVS_NamedObject.h"


namespace VieVS{

    /**
     * @class Bbc
     * @brief bbc section of observing mode
     *
     * CURRENTLY UNDER DEVELOPMENT AND UNUSED
     *
     * @author Matthias Schartner
     * @date 17.09.2018
     */
    class Bbc: public VieVS_NamedObject {
    public:

        explicit Bbc(std::string name);

        void addBbc(std::string name, unsigned int physical_bbc_number, std::string if_name);

        void toVexBbcDefinition( std::ofstream &of, const std::string &comment = "") const;

    private:
        static unsigned long nextId;

        class Bbc_assign: public VieVS_NamedObject{
        public:
            Bbc_assign(std::string name,
                       unsigned int physical_bbc_number,
                       std::string if_name):
                    VieVS_NamedObject{std::move(name), nextId++},
                    physical_bbc_number_{physical_bbc_number},
                    if_name_{std::move(if_name)}{};

            unsigned int physical_bbc_number_;
            std::string if_name_;
        private:
            static unsigned long nextId;

        };

        std::vector<Bbc_assign> bbc_assigns_;
    };
}


#endif //VIESCHEDPP_BBC_H
