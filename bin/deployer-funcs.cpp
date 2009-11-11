/***************************************************************************
 Copyright (c) 2008 S Roderick <xxxkiwi DOT xxxnet AT macxxx DOT comxxx>
                               (remove the x's above)

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 ***************************************************************************/

#include "deployer-funcs.hpp"
#include <rtt/Logger.hpp>
#ifdef  ORO_BUILD_RTALLOC
#include <stdio.h>
#endif
#include <iostream>
#include <sstream>
#include <string>
#include <functional>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace po = boost::program_options;

using namespace RTT;

namespace OCL
{

// map lowercase strings to levels
std::map<std::string, RTT::Logger::LogLevel>	logMap =
	boost::assign::map_list_of
	("never",       RTT::Logger::Debug)
	("fatal",       RTT::Logger::Fatal)
	("critical",    RTT::Logger::Critical)
	("error",       RTT::Logger::Error)
	("warning",     RTT::Logger::Warning)
	("info",        RTT::Logger::Info)
	("debug",       RTT::Logger::Debug)
	("realtime",    RTT::Logger::RealTime);

int deployerParseCmdLine(int                        argc,
                         char**                     argv,
                         std::string&               siteFile,
                         std::string&               scriptFile,
                         std::string&               name,
                         bool&                      requireNameService,
                         po::variables_map&         vm,
                         po::options_description*   otherOptions)
{
	std::string                         logLevel("info");	// set to valid default
	po::options_description             options;
	po::options_description             allowed("Allowed options");
	po::positional_options_description  pos;
	allowed.add_options()
		("help,h",
		 "Show program usage")
		("start,s",
		 po::value<std::string>(&scriptFile),
		 "Deployment configuration file (eg 'config-file.xml')")
		("site-file",
		 po::value<std::string>(&siteFile),
		 "Site deployment file (eg 'Deployer-site.cpf' or 'Deployer-site.xml')")
		("log-level,l",
		 po::value<std::string>(&logLevel),
		 "Level at which to log from RTT (case-insensitive) Never,Fatal,Critical,Error,Warning,Info,Debug,Realtime")
		("no-consolelog",
		 "Turn off RTT logging to the console (will still log to 'orocos.log')")
        ("require-name-service",
         "Require CORBA name service")
		("DeployerName",
		 po::value<std::string>(&name),
		 "Name of deployer component (the --DeployerName flag is optional)")
		;
	pos.add("DeployerName", 1);

	// collate options
	options.add(allowed);
	if (NULL != otherOptions)
	{
		options.add(*otherOptions);
	}

	try
	{
		po::store(po::command_line_parser(argc, argv).
                  options(options).positional(pos).run(),
                  vm);
		po::notify(vm);

        // deal with options
		if (vm.count("help"))
		{
			std::cout << options << std::endl;
			return 1;
		}

        // turn off all console logging
		if (vm.count("no-consolelog"))
		{
            RTT::Logger::Instance()->mayLogStdOut(false);
            log(Warning) << "Console logging disabled" << endlog();
		}

		if (vm.count("require-name-service"))
		{
            requireNameService = true;
            log(Info) << "CORBA name service required." << endlog();
		}

 		// verify that is a valid logging level
		boost::algorithm::to_lower(logLevel);	// always lower case
		if (vm.count("log-level"))
		{
			if (0 != logMap.count(logLevel))
			{
				RTT::Logger::Instance()->setLogLevel(logMap[logLevel]);
			}
			else
			{
				std::cout << "Did not understand log level: "
						  << logLevel << std::endl
						  << options << std::endl;
				return -1;
			}
		}
	}
	catch (std::logic_error e)
    {
		std::cerr << "Exception:" << e.what() << std::endl << options << std::endl;
        return -1;
    }

    return 0;
}

#ifdef  ORO_BUILD_RTALLOC

void validate(boost::any& v,
              const std::vector<std::string>& values,
              memorySize* target_type, int)
{
//    using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string& memSize = po::validators::get_single_string(values);

    /* parse the string. Support "number" or "numberX" where
       X is one of {k,K,m,M,g,G} with the expected meaning of X

       e.g.

       1024, 1k, 3.4M, 4.5G
    */
    float       value=0;
    char        units='\0';
    if (2 == sscanf(memSize.c_str(), "%f%c", &value, &units))
    {
        float       multiplier=1;
        if (islower(units))
        {
            units = toupper(units);
        }
        switch (units)
        {
            case 'G':
                multiplier *= 1024;
                // fall through
            case 'M':
                multiplier *= 1024;
                // fall through
            case 'K':
                multiplier *= 1024;
                break;
            default:
                std::stringstream   e;
                e << "Invalid units in rtalloc-mem-size option: " <<  memSize 
                  << ". Valid units: 'k','m','g' (case-insensitive).";
                throw po::validation_error(e.str());
        }
        value *= multiplier;
    }
    else if (1 == sscanf(memSize.c_str(), "%f", &value))
    {
        // nothing to do
    }
    else
    {
        throw po::validation_error("Could not parse rtalloc-mem-size option: " + memSize);
    }

    // provide some basic sanity checking
    // Note that TLSF has its own internal checks on the value.
    // TLSF's minimum size varies with build options, but it is
    // several kilobytes at least (6-8k on Mac OS X Snow Leopard
    // with 64-bit build, ~3k on Ubuntu Jaunty with 32-bit build).
    if (! (0 <= value) )
    {
        std::stringstream   e;
        e << "Invalid memory size of " << value << " given. Value must be >= 0.";
        throw po::validation_error(e.str());
    }

    v = memorySize((size_t)value);
}


boost::program_options::options_description deployerRtallocOptions(memorySize& rtallocMemorySize)
{
    boost::program_options::options_description rtallocOptions("Real-time memory allocation options");
    rtallocOptions.add_options()
		("rtalloc-mem-size",
         po::value<memorySize>(&rtallocMemorySize)->default_value(rtallocMemorySize),
		 "Amount of memory to provide for real-time memory allocations (e.g. 10000, 1K, 4.3m)\n"
         "NB the minimum size depends on TLSF build options, but it is several kilobytes.")
        ;
    return rtallocOptions;
}

#endif  //  ORO_BUILD_RTALLOC

// namespace
}
