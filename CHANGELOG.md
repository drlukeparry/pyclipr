
# Change Log
All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

### Fixed

### Changed

## [0.1.7] - 2024-01-11

### Added
- Updated build script to prepare source distribution (Linux)
- Script automatically pulls the specified submodule versions in setup.py

### Fixed
- Fixed old references to original build-scripts
- Fixed the external submodules (clipper2, pybind, eigen) to fixed version for reference
- Fixed a build issue with latest ClipperLib (PreserveCollinear property has become a protected member)

## [0.1.6] - 2023-12-16

### Added

### Fixed

- In `ClipperOffset::addPaths`, paths of each polygon are seperately constructed into a list of paths to ensure that these are correctly imported into ClipperOffset.
- Scale factor has been applied when importing paths

### Changed


## [0.1.4] - 2023-11-12

Update release for PyClipr and updated dependencies to ClipperLib2 v.1.2. and Eigen 3.4.0 - dependent on
the build system of Github. 

### Added
- Include the build option for Python 3.12 on Windows and Mac Release [8e84320de3f3c8d24840bcf031c518184385658b](https://github.com/drlukeparry/pyclipr/commit/8e84320de3f3c8d24840bcf031c518184385658b)
- Included examples for PyClipr usage in the examples folder  [a8bf872d6ada88b97e4e7be634ce25bd8079d0f4](https://github.com/drlukeparry/pyclipr/commit/a8bf872d6ada88b97e4e7be634ce25bd8079d0f4)

### Fixed

### Changed
- Update to BOOST license - permissive license for use in commercial applications - matching ClipperLib2 license - [300d5cf5df116d59877ebfe1de6a7a194728bce7](https://github.com/drlukeparry/pyclipr/commit/300d5cf5df116d59877ebfe1de6a7a194728bce7)
- `pyclipr.FillType` has been changed to pyclipr.FillRule for consistency with ClipperLib2 API - [978e4879a5bf9f8d0854fdd469868340efa38912](https://github.com/drlukeparry/pyclipr/commit/978e4879a5bf9f8d0854fdd469868340efa38912)
- Enumerations (`pyclipr.FillRule`, `pyclipr.EndType`, `pyclipr.JoinType`) have changed so their scope is restricted and not polluting global namespace  [978e4879a5bf9f8d0854fdd469868340efa38912](https://github.com/drlukeparry/pyclipr/commit/978e4879a5bf9f8d0854fdd469868340efa38912)

## [0.1.0] - 2023-07-14

  The first release of PyClipr in its distributed packaged form via PyPi. This release includes the basic functionality and refactoring to compile
