//
// Copyright 2010,2013-2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <vector>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string args, input_str, key, val;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "device address args [default = \"\"]")
        ("values", po::value<std::string>(&input_str), "keys+values to read/write, separate multiple by \",\"")
        ("key", po::value<std::string>(&key), "identifiers for new values in EEPROM, separate multiple by \",\" (DEPRECATED)")
        ("val", po::value<std::string>(&val), "the new values to set, omit for readback, separate multiple by \",\" (DEPRECATED)")
        ("read-all", "Read all motherboard EEPROM values without writing")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or (not vm.count("key") and not vm.count("values") and not vm.count("read-all"))){
        std::cout << boost::format("USRP Burn Motherboard EEPROM %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the value argument to perform a readback,\n"
            "Or specify a new value to burn into the EEPROM.\n"
        ) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Creating USRP device from address: " + args << std::endl;
    uhd::device::sptr dev = uhd::device::make(args);
    uhd::property_tree::sptr tree = dev->get_tree();
    uhd::usrp::mboard_eeprom_t mb_eeprom = tree->access<uhd::usrp::mboard_eeprom_t>("/mboards/0/eeprom").get();
    std::cout << std::endl;

    std::vector<std::string> keys_vec, vals_vec;
    if(vm.count("read-all")) keys_vec = mb_eeprom.keys(); //Leaving vals_vec empty will force utility to only read
    else if(vm.count("values")){
        //uhd::device_addr_t properly parses input values
        uhd::device_addr_t vals(input_str);
        keys_vec = vals.keys();
        vals_vec = vals.vals();
    }
    else{
        std::cout << "WARNING: Use of --key and --val is deprecated!" << std::endl;
        //remove whitespace, split arguments and values 
        boost::algorithm::erase_all(key, " ");
        boost::algorithm::erase_all(val, " ");

        boost::split(keys_vec, key, boost::is_any_of("\"',"));
        boost::split(vals_vec, val, boost::is_any_of("\"',"));

        if((keys_vec.size() != vals_vec.size()) and val != "") {
            //If zero values are given, then user just wants values read to them
            throw std::runtime_error("Number of keys must match number of values!");
        }
    }

    std::cout << "Fetching current settings from EEPROM..." << std::endl;
    for(size_t i = 0; i < keys_vec.size(); i++){
        if (not mb_eeprom.has_key(keys_vec[i])){
            std::cerr << boost::format("Cannot find value for EEPROM[%s]") % keys_vec[i] << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << boost::format("    EEPROM [\"%s\"] is \"%s\"") % keys_vec[i] % mb_eeprom[keys_vec[i]] << std::endl;
    }
    std::cout << std::endl;
    for(size_t i = 0; i < vals_vec.size(); i++){
        if(vals_vec[i] != ""){
            uhd::usrp::mboard_eeprom_t mb_eeprom; mb_eeprom[keys_vec[i]] = vals_vec[i];
            std::cout << boost::format("Setting EEPROM [\"%s\"] to \"%s\"...") % keys_vec[i] % vals_vec[i] << std::endl;
            tree->access<uhd::usrp::mboard_eeprom_t>("/mboards/0/eeprom").set(mb_eeprom);
        }
    }
    std::cout << "Power-cycle the USRP device for the changes to take effect." << std::endl;
    std::cout << std::endl;

    std::cout << "Done" << std::endl;
    return EXIT_SUCCESS;
}
