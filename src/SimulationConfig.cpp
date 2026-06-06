#include "SimulationConfig.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

std::string trim(const std::string &s) {
  size_t start = 0;
  while (start < s.size() &&
         std::isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }

  size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }

  return s.substr(start, end - start);
}

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

bool parseInt(const std::string &s, int &out) {
  std::istringstream iss(s);
  int value;
  iss >> value;
  if (iss.fail() || !iss.eof())
    return false;
  out = value;
  return true;
}

bool parseFloat(const std::string &s, float &out) {
  std::istringstream iss(s);
  float value;
  iss >> value;
  if (iss.fail() || !iss.eof())
    return false;
  out = value;
  return true;
}

} // namespace

SimulationConfig loadSimulationConfig(const std::string &path) {
  SimulationConfig cfg;

  std::ifstream in(path);
  if (!in) {
    return cfg;
  }

  std::string line;
  while (std::getline(in, line)) {
    size_t commentPos = line.find('#');
    if (commentPos != std::string::npos) {
      line = line.substr(0, commentPos);
    }

    line = trim(line);
    if (line.empty())
      continue;

    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos)
      continue;

    std::string key = toLower(trim(line.substr(0, eqPos)));
    std::string value = trim(line.substr(eqPos + 1));

    if (key == "shape") {
      std::string shapeName = toLower(value);
      if (shapeName == "needle" || shapeName == "needle_plate" ||
          shapeName == "needleplate") {
        cfg.shape = ShapeKind::NeedlePlate;
      } else if (shapeName == "circle") {
        cfg.shape = ShapeKind::Circle;
      }
      continue;
    }

    int intValue = 0;
    float floatValue = 0.0f;

    if (key == "needle.length" && parseInt(value, intValue))
      cfg.needleLength = intValue;
    else if (key == "needle.half_width" && parseInt(value, intValue))
      cfg.needleHalfWidth = intValue;
    else if (key == "needle.top_offset" && parseInt(value, intValue))
      cfg.needleTopOffset = intValue;
    else if (key == "needle.plate_thickness" && parseInt(value, intValue))
      cfg.needlePlateThickness = intValue;
    else if (key == "needle.potential" && parseFloat(value, floatValue))
      cfg.needlePotential = floatValue;
    else if (key == "plate.potential" && parseFloat(value, floatValue))
      cfg.platePotential = floatValue;
    else if (key == "circle.thickness" && parseInt(value, intValue))
      cfg.circleThickness = intValue;
    else if (key == "circle.center_potential" && parseFloat(value, floatValue))
      cfg.circleCenterPotential = floatValue;
    else if (key == "circle.outer_potential" && parseFloat(value, floatValue))
      cfg.circleOuterPotential = floatValue;
  }

  return cfg;
}
