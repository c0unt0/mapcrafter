/*
 * Copyright 2012-2014 Moritz Hilscher
 *
 * This file is part of Mapcrafter.
 *
 * Mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mapcraftercore/renderer/manager.h"
#include "mapcraftercore/util.h"
#include "mapcraftercore/version.h"

#include <iostream>
#include <string>
#include <cstring>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// evil, I know
using namespace mapcrafter;

template<class T>
po::typed_value<T>* value(T* v, const char* value_typename) {
    po::typed_value<T>* r = po::value<T>(v);
    r->value_name(value_typename);
    return r;
}

int main(int argc, char** argv) {
	fs::path logging_config;
	std::string color;
	fs::path config_file;
	std::vector<std::string> render_skip, render_auto, render_force;
	int jobs;

	po::options_description general("General options");
	general.add_options()
		("help,h", "shows this help message")
		("version,v", "shows the version of Mapcrafter");

	po::options_description logging("Logging/output options");
	logging.add_options()
		("logging-config", value<fs::path>(&logging_config, "path"),
			"global logging configuration file (automatically determined of not specified)")
		("color", value<std::string>(&color, "colored")->default_value("auto"),
			"whether logging output is colored (true, false or auto)")
		("batch,b", "deactivates the animated progress bar and enables the progress logger instead");

	po::options_description renderer("Renderer options");
	renderer.add_options()
		("find-resources", "shows available resource directories")
		("config,c", value<fs::path>(&config_file, "path"),
			"the path to the configuration file to use (required)")
		("render-skip,s", value<std::vector<std::string>>(&render_skip, "maps")->multitoken(),
			"skips rendering the specified map(s)")
		("render-reset,r", "skips rendering all maps")
		("render-auto,a", value<std::vector<std::string>>(&render_auto, "maps")->multitoken(),
			"renders the specified map(s)")
		("render-force,f", value<std::vector<std::string>>(&render_force, "maps")->multitoken(),
			"renders the specified map(s) completely")
		("jobs,j", value<int>(&jobs, "number")->default_value(1),
			"the count of jobs to render the map");

	po::options_description all("Allowed options");
	all.add(general).add(logging).add(renderer);

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, all), vm);
	} catch (po::error& ex) {
		std::cerr << "There is a problem parsing the command line arguments: "
				<< ex.what() << std::endl;
		std::cerr << "Use '" << argv[0] << " --help' for more information." << std::endl;
		return 1;
	}

	po::notify(vm);

	if (vm.count("help")) {
		std::cout << all;
		return 0;
	}

	if (vm.count("version")) {
		std::cout << "Mapcrafter version: " << MAPCRAFTER_VERSION;
		if (strlen(MAPCRAFTER_GITVERSION))
			std::cout << " (" << MAPCRAFTER_GITVERSION << ")";
		std::cout << std::endl;
		return 0;
	}

	if (vm.count("find-resources")) {
		fs::path mapcrafter_bin = util::findExecutablePath();
		std::cout << "Your home directory: " << util::findHomeDir().string() << std::endl;
		std::cout << "Mapcrafter binary: " << mapcrafter_bin.string() << std::endl;
		
		util::PathList resources = util::findResourceDirs(mapcrafter_bin);
		std::cout << "Resource directories:" << std::endl;
		for (size_t i = 0; i < resources.size(); i++)
			std::cout << "  " << i+1 << ". " << BOOST_FS_ABSOLUTE1(resources[i]).string() << std::endl;
		if (resources.size() == 0)
			std::cout << "  Nothing found." << std::endl;

		util::PathList templates = util::findTemplateDirs(mapcrafter_bin);
		std::cout << "Template directories:" << std::endl;
		for (size_t i = 0; i < templates.size(); i++)
			std::cout << "  " << i+1 << ". " << BOOST_FS_ABSOLUTE1(templates[i]).string() << std::endl;
		if (templates.size() == 0)
			std::cout << "  Nothing found." << std::endl;

		util::PathList textures = util::findTextureDirs(mapcrafter_bin);
		std::cout << "Texture directories:" << std::endl;
		for (size_t i = 0; i < textures.size(); i++)
			std::cout << "  " << i+1 << ". " << BOOST_FS_ABSOLUTE1(textures[i]).string() << std::endl;
		if (textures.size() == 0)
			std::cout << "  Nothing found." << std::endl;

		util::PathList configs = util::findLoggingConfigFiles(mapcrafter_bin);
		std::cout << "Logging configuration file:" << std::endl;
		for (size_t i = 0; i < configs.size(); i++)
			std::cout << "  " << i+1 << ". " << BOOST_FS_ABSOLUTE1(configs[i]).string() << std::endl;
		if (configs.size() == 0)
			std::cout << "  Nothing found." << std::endl;
		return 0;
	}

	if (!vm.count("config")) {
		std::cerr << "You have to specify a config file!" << std::endl;
		std::cerr << "Use '" << argv[0] << " --help' for more information." << std::endl;
		return 1;
	}

	renderer::RenderOpts opts;
	opts.config_file = config_file.string();
	opts.render_skip = render_skip;
	opts.skip_all = vm.count("render-reset");
	opts.render_auto = render_auto;
	opts.render_force = render_force;
	opts.jobs = jobs;
	opts.batch = vm.count("batch");
	renderer::RenderManager manager(opts);
	if (!manager.run())
		return 1;
	return 0;
}
