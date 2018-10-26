/* Copyright 2018 - Stefano Sinigardi, Alessandro Fabbri */

/***************************************************************************
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
***************************************************************************/

#include <iostream>

#include <jsoncons/json.hpp>

#define SW_VER_CROWD 101
#define NUMBER_OF_FILES 3

class Json_data {
  public:
  unsigned int timestamp;
  std::string id_box;
  std::string detection;
  unsigned short sw_ver;
  unsigned int people_count;
};

void usage(char* progname)
{
  std::cout << "Usage: " << progname << " basename_json_triplet" << std::endl;
  std::cout << "       Tool will open basename_json_triplet.json, basename_json_triplet.1.json and basename_json_triplet.2.json" << std::endl;
  std::cout << "       and it will produce a single json with the maximum value for the \"people_count\" key" << std::endl;
  exit(-1);
}

int max_of_three(int i, int j, int k)
{
  int result = i;
  if (j > result)
    result = j;
  if (k > result)
    result = k;
  return result;
}

int min_of_three(int i, int j, int k)
{
  int result = i;
  if (j < result)
    result = j;
  if (k < result)
    result = k;
  return result;
}

int main(int argc, char** argv)
{
  std::vector<std::string> input_files;
  if (argc != 2) {
    usage(argv[0]);
  }

  input_files.push_back(std::string(argv[1]) + ".json");
  input_files.push_back(std::string(argv[1]) + ".1.json");
  input_files.push_back(std::string(argv[1]) + ".2.json");

  std::string out_file = std::string(argv[1]) + ".json";

  std::cout << "Packing: ";
  for (auto i : input_files)
    std::cout << i << "  ";

  std::cout << std::endl
            << "Output: " << out_file << std::endl;

  Json_data json_data[NUMBER_OF_FILES];

  int counter = 0;
  for (int i = 0; i < NUMBER_OF_FILES; ++i) {
    jsoncons::json in_json;
    try {
      in_json = jsoncons::json::parse_file(input_files[i]);
    } catch (std::exception& e) {
      std::cout << "EXCEPTION: " << e.what() << std::endl;
      continue;
    }

    json_data[i].timestamp = in_json["timestamp"].as<unsigned int>();
    json_data[i].id_box = in_json["id_box"].as<std::string>();
    json_data[i].detection = in_json["detection"].as<std::string>();
    json_data[i].sw_ver = in_json["sw_ver"].as<unsigned short>();

    jsoncons::json in_people_count = in_json["people_count"];
    for (auto pc : in_people_count.array_range()) {
      json_data[i].people_count = pc["count"].as<unsigned int>();
    }
    jsoncons::json in_diagnostics = in_json["diagnostics"];
  }

  unsigned int max_person_number = max_of_three(json_data[0].people_count, json_data[1].people_count, json_data[2].people_count);
  unsigned int min_person_number = min_of_three(json_data[0].people_count, json_data[1].people_count, json_data[2].people_count);

  jsoncons::json out_json;

  out_json["timestamp"] = json_data[0].timestamp;
  out_json["id_box"] = json_data[0].id_box;
  out_json["detection"] = json_data[0].detection;
  out_json["sw_ver"] = SW_VER_CROWD;

  jsoncons::json people_count;
  people_count["id"] = json_data[0].id_box;
  people_count["count"] = max_person_number;
  out_json["people_count"] = people_count;

  jsoncons::json diagnostics;
  std::string diagnostics_tag = "minimum_detected";
  diagnostics["id"] = diagnostics_tag;
  diagnostics["value"] = min_person_number;
  out_json["diagnostics"] = diagnostics;

  std::ofstream output(out_file);
  output << jsoncons::pretty_print(out_json);
  output.close();

  return 0;
}
