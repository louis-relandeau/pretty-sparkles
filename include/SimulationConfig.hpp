#pragma once

#include <string>

enum class ShapeKind { NeedlePlate, Circle };

struct SimulationConfig {
  ShapeKind shape = ShapeKind::NeedlePlate;

  int needleLength = 90;
  int needleHalfWidth = 1;
  int needleTopOffset = 2;
  int needlePlateThickness = 2;
  float needlePotential = 0.0f;
  float platePotential = 1.0f;

  int circleThickness = 5;
  float circleCenterPotential = 1.0f;
  float circleOuterPotential = 0.0f;
};

SimulationConfig loadSimulationConfig(const std::string &path);
