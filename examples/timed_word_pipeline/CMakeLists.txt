cmake_minimum_required(VERSION 3.25)
project(PycircuitTimedWordPipeline LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

# word_arithmetic.py is the existing public functional_float source unit.
pycircuit_add_example(timed_word_pipeline
  PACKAGE timed_word_pipeline
  TOP timed_word_pipeline.bench.TimedWordPipelineSystem
  ENTRY_SOURCE bench
  PROVIDER_UNITS "../functional_float|functional_float|word_arithmetic.py"
  SOURCES timed_word_pipeline.py bench.py
  MODEL_SOURCES timed_word_pipeline.py alternate_model.py
  CYCLES 24)
