#include <string>
#include <tuple>

#include <Eigen/Eigen>

#include "clipper2/clipper.h"

#include <pybind11/pybind11.h>

#include <pybind11/numpy.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/eigen.h>

//using namespace Clipper2Lib;

namespace py = pybind11;

namespace pyclipr {

typedef Eigen::Matrix<uint64_t,Eigen::Dynamic,2> EigenVec2i;
typedef Eigen::Matrix<double,Eigen::Dynamic,1>   EigenVec1d;
typedef Eigen::Matrix<double,Eigen::Dynamic,2>   EigenVec2d;
typedef Eigen::Matrix<double,Eigen::Dynamic,3>   EigenVec3d;

static void myZCB(const Clipper2Lib::Point64& e1bot, const Clipper2Lib::Point64& e1top,
                  const Clipper2Lib::Point64& e2bot, const Clipper2Lib::Point64& e2top,
                  Clipper2Lib::Point64& pt) {
    // Find the maximum z value from all points. Using a background value of Zero for the contour allows, individual data
    // to be isolated.

    // Assume contour or clipping polygons has the lowest value
    int64_t maxZ = std::numeric_limits<int64_t>::lowest();

    if(e1bot.z > maxZ)
        maxZ = e1bot.z;

    if(e1top.z > maxZ)
        maxZ = e1top.z;

    if(e2top.z > maxZ)
        maxZ = e2top.z;

    if(e2bot.z > maxZ)
        maxZ = e2bot.z;

    // Assign the z value to pt
    pt.z = maxZ;
};

pyclipr::EigenVec2d path2EigenVec2d(const Clipper2Lib::Path64 &path, double scaleFactor) {

    pyclipr::EigenVec2d eigPath(path.size(), 2);

    for (uint64_t i=0; i<path.size(); i++) {
        eigPath(i,0) = path[i].x;
        eigPath(i,1) = path[i].y;
    }
    eigPath *= scaleFactor;

    return eigPath;
}

Clipper2Lib::Path64 createPath(const py::array_t<double> &path, double scaleFactor)
{

    if (path.ndim() != 2)
        throw std::runtime_error("Number of dimensions must be two");

    if (!(path.shape(1) == 2 || path.shape(1) == 3))
        throw std::runtime_error("Path must be nx2, or nx3");

    auto r = path.unchecked<2>();

    Clipper2Lib::Path64 p;

    // Resize the path list
    p.reserve(path.shape(0));

    if(path.shape(1) == 2) {
        for(uint64_t i=0; i < path.shape(0); i++)
            p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor));

    } else {
        for(uint64_t i=0; i < path.shape(0); i++)
            p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor, r(i,2)));

    }
    return p;
}

std::vector<Clipper2Lib::Path64> createPaths(const std::vector<py::array_t<double>> &paths, double scaleFactor)  {

    /* Check all array of paths have a consistent size and dimension */

    int numDims = paths[0].ndim();
    int numCols = paths[0].shape(1);

    for (auto path: paths) {
        if (path.ndim() != numDims)
            throw std::runtime_error("All paths must have same numpy dimensions");

        if (path.shape(1) != numCols)
            throw std::runtime_error("All paths must have same number of dimensions");
    }

    std::vector<Clipper2Lib::Path64> clipperPaths;
    for (auto path: paths) {
        const Clipper2Lib::Path64 & cPath = createPath(path, scaleFactor);
        clipperPaths.push_back(cPath);
    }

    return clipperPaths;
}

py::object simplifyPath(const py::array_t<double> &path, double epsilon, double scaleFactor = 1e3, bool isOpenPath = false)
{
    if (scaleFactor < std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("Scale factor cannot be zero");
    }

    Clipper2Lib::Path64 clipperPath = createPath(path, scaleFactor);
    Clipper2Lib::Path64 simplifiedPath = Clipper2Lib::SimplifyPath(clipperPath, epsilon, isOpenPath);

    // Convert this back into an eigen Vector2D
    EigenVec2d simpPathOut = path2EigenVec2d(simplifiedPath, 1.0 / scaleFactor);
    return py::cast(simpPathOut);
}

py::object simplifyPaths(std::vector<py::array_t<double>> &paths, double epsilon, double scaleFactor = 1e3, bool isOpenPath = false)
{
    if (scaleFactor < std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("Scale factor cannot be zero");
    }

    std::vector<Clipper2Lib::Path64> clipperPaths = createPaths(paths, scaleFactor);
    std::vector<Clipper2Lib::Path64> simpPaths =  Clipper2Lib::SimplifyPaths(clipperPaths, epsilon, isOpenPath);

    std::vector<EigenVec2d> simpPathsOut;
    for (auto simpPath : simpPaths) {

        // Convert this back into an eigen Vector2D
        EigenVec2d simpPathOut = path2EigenVec2d(simpPath, 1.0 / scaleFactor);
        simpPathsOut.push_back(simpPathOut);
    }

    return py::cast(simpPathsOut);
}

bool orientation(const py::array_t<double> &path, const double scaleFactor = 1e3)
{
    if (scaleFactor < std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("Scale factor cannot be zero");
    }

    Clipper2Lib::Path64 p = createPath(path, scaleFactor);

    return Clipper2Lib::IsPositive(p);
}

py::object polyTreeToPaths64(const Clipper2Lib::PolyTree64 &polytree, double scaleFactor = 1.0)
{
    auto paths = Clipper2Lib::PolyTreeToPaths64(polytree);

    std::vector<EigenVec2d> closedOut;

    for (auto &path : paths) {
        auto eigPath = path2EigenVec2d(path, scaleFactor);
        closedOut.push_back(eigPath);
    }

    return py::cast(closedOut);
}

py::object polyTreeToPathsD(const Clipper2Lib::PolyPathD &polytree)
{
    auto paths = Clipper2Lib::PolyTreeToPathsD(polytree);

    std::vector<EigenVec2d> closedOut;

    for (auto &path : paths) {

        EigenVec2d eigPath(path.size(), 2);

        for (uint64_t i=0; i<path.size(); i++) {
            eigPath(i,0) = double(path[i].x);
            eigPath(i,1) = double(path[i].y);
        }

        closedOut.push_back(eigPath);
    }

    return py::cast(closedOut);
}

void applyScaleFactor(const Clipper2Lib::PolyPath64 & polyPath,
                      Clipper2Lib::PolyPathD &newPath, double scaleFactor, bool invert = true) {
    // Create a recursive copy of the structure

    for(uint64_t i = 0; i < polyPath.Count(); i++)
    {
        Clipper2Lib::Path64 path = polyPath[i]->Polygon();
        Clipper2Lib::PolyPathD pathD;
        newPath.SetScale(1.0 / scaleFactor);

        auto newChild = newPath.AddChild(path);

        applyScaleFactor(*(polyPath[i]), *newChild, scaleFactor);

    }
}


class Clipper : public Clipper2Lib::Clipper64 {

public:

    Clipper() : Clipper2Lib::Clipper64(), scaleFactor(1000.0)
    {
        this->SetZCallback(myZCB);
    }
    ~Clipper() {}


public:

   void addPath(const py::array_t<double> &path,
                Clipper2Lib::PathType polyType,
                bool isOpen)
    {
        Clipper2Lib::Path64 p;

        if (path.ndim() != 2)
            throw std::runtime_error("Number of dimensions must be two");

        if (!(path.shape(1) == 2 || path.shape(1) == 3))
            throw std::runtime_error("Path must be nx2, or nx3");

        // Resize the path list
        p.reserve(path.shape(0));

        auto r = path.unchecked<2>();

        if(path.shape(1) == 2) {
            for(uint64_t i=0; i < path.shape(0); i++)
                p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor));

        } else {
            for(uint64_t i=0; i < path.shape(0); i++)
                p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor, r(i,2)));

        }

        this->AddPath(p, polyType, isOpen);
    }

    void addPaths(const std::vector<pybind11::array_t<double>> paths,
                  const Clipper2Lib::PathType polyType,
                  bool isOpen)
    {
        for(auto path : paths)
            addPath(path, polyType, isOpen);
    }

    void cleanUp() { this->CleanUp(); }
    void clear() { this->Clear(); }

    void setScaleFactor(const double scaleFactor) {
        if (scaleFactor < std::numeric_limits<double>::epsilon()) {
            throw std::runtime_error("Scale factor cannot be zero");
        }

        this->scaleFactor = scaleFactor;
    }
    void setPreserveCollinear(bool val) { this->PreserveCollinear(val); }

    double getScaleFactor() const {  return this->scaleFactor; }
    bool getPreserveCollinear() const { return this->PreserveCollinear(); }

    py::object execute(const Clipper2Lib::ClipType clipType, const Clipper2Lib::FillRule fillRule,
                       bool returnOpenPaths = false, bool returnZ = false) {

        Clipper2Lib::Paths64 closedPaths;
        Clipper2Lib::Paths64 openPaths;

        this->Execute(clipType, fillRule, closedPaths, openPaths);

        std::vector<EigenVec2d> closedOut;
        std::vector<EigenVec1d> closedOutZ;
        std::vector<EigenVec2d> openOut;
        std::vector<EigenVec1d> openOutZ;

        for (auto &path : closedPaths) {

            EigenVec2d eigPath(path.size(), 2);
            EigenVec1d eigPathZ(path.size(), 1);

            for (uint64_t i=0; i < path.size(); i++) {
                eigPath(i,0) = double(path[i].x) / double(scaleFactor);
                eigPath(i,1) = double(path[i].y) / double(scaleFactor);

                if(returnZ)
                    eigPathZ(i, 0) = path[i].z;
            }

            closedOut.push_back(eigPath);

            if(returnZ)
                closedOutZ.push_back(eigPathZ);
        }

        if(!returnOpenPaths) {
            if(returnZ) {
                return py::make_tuple(closedOut, closedOutZ);
            } else {
                return py::cast(closedOut);
            }
        } else {

            for (auto &path : openPaths) {

                EigenVec1d eigPathZ(path.size(), 1);
                EigenVec2d eigPath(path.size(), 2);

                for (uint64_t i=0; i<path.size(); i++) {
                    eigPath(i,0) = double(path[i].x) / double(scaleFactor);
                    eigPath(i,1) = double(path[i].y) / double(scaleFactor);

                    if(returnZ) {
                        eigPathZ(i) = path[i].z;
                    }
                }

                openOut.push_back(eigPath);
                openOutZ.push_back(eigPathZ);
            }

            if(returnZ) {
                return py::make_tuple(closedOut, openOut, closedOutZ, openOutZ);
            } else {
                return py::make_tuple(closedOut, openOut);
            }
        }
    }


    py::object execute2(const Clipper2Lib::ClipType clipType, const Clipper2Lib::FillRule fillRule,
                        const bool returnOpenPaths = false, const bool returnZ = false) {

        Clipper2Lib::PolyPath64 polytree;
        Clipper2Lib::Paths64 openPaths;

        this->Execute(clipType, fillRule, polytree, openPaths);

        Clipper2Lib::PolyPathD *polytreeCpy = new Clipper2Lib::PolyPathD();

        applyScaleFactor(polytree, *polytreeCpy, scaleFactor);

        if(returnOpenPaths) {

            std::vector<EigenVec2d> openPathOut;
            std::vector<EigenVec1d> openPathOutZ;

            for (auto &path : openPaths) {

                EigenVec2d eigPath(path.size(), 2);
                EigenVec1d eigPathZ(path.size(), 1);

                for (uint64_t i=0; i<path.size(); i++) {
                    eigPath(i,0) = double(path[i].x) / double(scaleFactor);
                    eigPath(i,1) = double(path[i].y) / double(scaleFactor);

                    if(returnZ)
                        eigPathZ(i) = path[i].z;
                }

                openPathOut.push_back(eigPath);
                openPathOutZ.push_back(eigPathZ);

            }

            if(returnZ) {
                return py::make_tuple(polytreeCpy, openPathOut, openPathOutZ);
            } else {
                return py::make_tuple(polytreeCpy, openPathOut);
            }

        } else {
            return  py::cast(polytreeCpy);
        }

    }

protected:
    double scaleFactor;
};


class ClipperOffset : public Clipper2Lib::ClipperOffset {

public:

    ClipperOffset() : Clipper2Lib::ClipperOffset(), scaleFactor(1000.0)
    {

    }
    ~ClipperOffset() {}

public:

    void addPath(const pybind11::array_t<double>& path,
                            const Clipper2Lib::JoinType joinType,
                            const Clipper2Lib::EndType endType = Clipper2Lib::EndType::Polygon)
    {
        Clipper2Lib::Path64 p;

        if (path.ndim() != 2)
            throw std::runtime_error("Number of dimensions must be two");

        if (!(path.shape(1) == 2 || path.shape(1) == 3))
            throw std::runtime_error("Path must be nx2, or nx3");

        // Resize the path list
        p.reserve(path.shape(0));

        auto r = path.unchecked<2>();

        if(path.shape(1) == 2) {
            for(uint64_t i=0; i < path.shape(0); i++)
                p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor));
        } else {
            for(uint64_t i=0; i < path.shape(0); i++)
                p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor, r(i,2)));
        }

        this->AddPath(p, joinType, endType);

    }

    void setScaleFactor(const double scaleFactor) {
        if (scaleFactor < std::numeric_limits<double>::epsilon()) {
            throw std::runtime_error("Scale factor cannot be zero");
        }

        this->scaleFactor = scaleFactor;
    }

    double getScaleFactor() const {  return this->scaleFactor; }
    double getArcTolerance() const { return this->ArcTolerance(); }
    double getMiterLimit() const { return this->MiterLimit(); }
    bool getPreserveCollinear() const { return this->PreserveCollinear(); }

    void setMiterLimit(double val) { this->MiterLimit(val); }
    void setArcTolerance(double val) { this->ArcTolerance(val); }
    void setPreserveCollinear(bool val) { this->PreserveCollinear(val); }

    void addPaths(const std::vector<pybind11::array_t<double>> paths,
                  const Clipper2Lib::JoinType joinType,
                  const Clipper2Lib::EndType endType = Clipper2Lib::EndType::Polygon)
    {

        std::vector<Clipper2Lib::Path64> closedPaths;

        for(auto path : paths) {

            Clipper2Lib::Path64 p;

            if (path.ndim() != 2)
                throw std::runtime_error("Number of dimensions must be two");

            if (!(path.shape(1) == 2 || path.shape(1) == 3))
                throw std::runtime_error("Path must be nx2, or nx3");

            // Resize the path list
            p.reserve(path.shape(0));

            auto r = path.unchecked<2>();

            if(path.shape(1) == 2) {
                for(uint64_t i=0; i < path.shape(0); i++)
                    p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor));
            } else {
                for(uint64_t i=0; i < path.shape(0); i++)
                    p.push_back(Clipper2Lib::Point64(r(i,0) * scaleFactor, r(i,1) * scaleFactor, r(i,2)));
            }

            closedPaths.push_back(p);

        }
        this->AddPaths(closedPaths, joinType, endType);
    }

    void clear() { this->Clear(); }

    py::object  execute(const double delta) {

        Clipper2Lib::Paths64 closedPaths;

        this->Execute(delta * scaleFactor, closedPaths);

        std::vector<EigenVec2d> closedOut;

        for (auto &path : closedPaths) {

            EigenVec2d eigPath(path.size(), 2);

            for (uint64_t i=0; i<path.size(); i++) {
                eigPath(i,0) = double(path[i].x) / double(scaleFactor);
                eigPath(i,1) = double(path[i].y) / double(scaleFactor);
            }

            closedOut.push_back(eigPath);
        }

        return py::cast(closedOut);
    }

    Clipper2Lib::PolyPathD * execute2(const double delta) {

        Clipper2Lib::PolyPath64 polytree;

        this->Execute(delta * scaleFactor, polytree);

        Clipper2Lib::PolyPathD *polytreeCpy = new Clipper2Lib::PolyPathD();

        applyScaleFactor(polytree, *polytreeCpy, scaleFactor);

        return polytreeCpy;
    }

protected:
    double scaleFactor;
};

} // end of namespace pyclipr

PYBIND11_MODULE(pyclipr, m, py::mod_gil_not_used()) {

    m.doc() = R"pbdoc(
        PyClipr Module
        -----------------------
        .. currentmodule:: pyclipr
        .. autosummary::
           :toctree: _generate

    )pbdoc";


	m.attr("clipperVersion")= CLIPPER2_VERSION;

    py::class_<Clipper2Lib::PolyPath>(m, "PolyPath")
        /*.def(py::init<>()) */
        .def_property_readonly("level", &Clipper2Lib::PolyPath::Level)
        .def_property_readonly("parent", &Clipper2Lib::PolyPath::Parent);

    py::class_<Clipper2Lib::PolyPath64>(m, "PolyTree")
        /* .def(py::init<>()) */
        .def_property_readonly("isHole", &Clipper2Lib::PolyPath64::IsHole)
        .def_property_readonly("area",   &Clipper2Lib::PolyPath64::Area)
        .def_property_readonly("attributes", [](const Clipper2Lib::PolyPath64 &s ) -> py::object {
            Clipper2Lib::Path64 path = s.Polygon();

            pyclipr::EigenVec1d eigPath(path.size(), 1);

            for (uint64_t i=0; i<path.size(); i++)
                eigPath(i,0) = path[i].z;

            return py::cast(eigPath);
        })
        .def_property_readonly("polygon", [](const Clipper2Lib::PolyPath64 &s ) -> py::object {
            Clipper2Lib::Path64 path = s.Polygon();

            pyclipr::EigenVec2d eigPath(path.size(), 3);

            for (uint64_t i=0; i<path.size(); i++) {
                eigPath(i,0) = path[i].x;
                eigPath(i,1) = path[i].y;
                //eigPath(i,2) = path[i].z;
            }
            return py::cast(eigPath);
        })
        .def_property_readonly("children", [](const Clipper2Lib::PolyPath64 &s ) {
            std::vector<const Clipper2Lib::PolyPath64 *> children;
            for (int i = 0; i < s.Count(); i++) {
                children.push_back(s[i]);
            }

             return children;
          })
        .def_property_readonly("count", &Clipper2Lib::PolyPath64::Count)
        .def("__len__", [](const Clipper2Lib::PolyTree64 &s ) { return s.Count(); });


    py::class_<Clipper2Lib::PolyPathD>(m, "PolyTreeD")
            /* .def(py::init<>()) */
            .def_property_readonly("isHole", &Clipper2Lib::PolyPathD::IsHole)
            .def_property_readonly("area",   &Clipper2Lib::PolyPathD::Area)
            .def_property_readonly("attributes", [](const Clipper2Lib::PolyPathD &s) -> py::object {
                Clipper2Lib::PathD path = s.Polygon();

                pyclipr::EigenVec1d eigPath(path.size(), 1);

                for (uint64_t i=0; i<path.size(); i++)
                    eigPath(i,0) = path[i].z;

                return py::cast(eigPath);
            })
            .def_property_readonly("polygon", [](const Clipper2Lib::PolyPathD &s ) ->  py::object {
                Clipper2Lib::PathD path = s.Polygon();

                pyclipr::EigenVec2d eigPath(path.size(), 2);

                for (uint64_t i=0; i<path.size(); i++) {
                    eigPath(i,0) = path[i].x;
                    eigPath(i,1) = path[i].y;
                }
                return py::cast(eigPath);
            })
            .def_property_readonly("children", [](const Clipper2Lib::PolyPathD &s ) {
                std::vector<const Clipper2Lib::PolyPathD *> children;
                for (int i = 0; i < s.Count(); i++)
                    children.push_back(s[i]);

                return children;
            })
            .def_property_readonly("count", &Clipper2Lib::PolyPathD::Count)
            .def("__len__", [](const Clipper2Lib::PolyPathD &s ) { return s.Count(); });


    m.def("polyTreeToPaths64", &pyclipr::polyTreeToPaths64, py::return_value_policy::automatic)
     .def("orientation", &pyclipr::orientation, py::arg("path"),  py::arg("scaleFactor") = 1000, py::return_value_policy::automatic, R"(
        This function returns the orientation of a path. Orientation will return `True` if the polygon's orientation
        is counter-clockwise.

        :param path: A 2D numpy array of shape (n, 2) or (n, 3) where n is the number of vertices in the path.
        :param scaleFactor: Optional scale factor for the internal clipping factor. Defaults to 1000.
        :return: `True` if the polygon's orientation is counter-clockwise, `False` otherwise.
        )" )
    .def("polyTreeToPaths", &pyclipr::polyTreeToPaths64, py::return_value_policy::automatic)
    .def("simplifyPath", &pyclipr::simplifyPath, py::arg("path"), py::arg("epsilon"), py::arg("scaleFactor")  = 1000, py::arg("isOpenPath") = false,
                        py::return_value_policy::automatic, R"(
            This function removes vertices that are less than the specified epsilon distance from an imaginary line
            that passes through its two adjacent vertices. Logically, smaller epsilon values will be less aggressive
            in removing vertices than larger epsilon values.

            :param path: A 2D numpy array of shape (n, 2) or (n, 3) where n is the number of vertices in the path.
            :param epsilon: The maximum distance a vertex can be from an imaginary line that passes through its two adjacent vertices.
            :param scaleFactor: The scaleFactor applied to the path during simplification
            :param isOpenPath: If `True`, the path is treated as an open path. If `False`, the path is treated as a closed path.
            :return: Simplified path
            )"
     )
    .def("simplifyPaths", &pyclipr::simplifyPaths, py::arg("paths"), py::arg("epsilon"),
                                                   py::arg("scaleFactor")  = 1000, py::arg("isOpenPath") = false,
                                                   py::return_value_policy::automatic, R"(
            This function removes vertices that are less than the specified epsilon distance from an imaginary line
            that passes through its two adjacent vertices. Logically, smaller epsilon values will be less aggressive
            in removing vertices than larger epsilon values.

            :param paths: A list of 2D points (x,y) that define the path. Tuple or a numpy array may be provided for the path
            :param epsilon: The maximum distance a vertex can be from an imaginary line that passes through its 2 adjacent vertices.
            :param scaleFactor: The scaleFactor applied to the path during simplification
            :param isOpenPath: If `True`, the path is treated as an open path. If `False`, the path is treated as a closed path.
            :return: None
            )"
    );


    py::class_<pyclipr::Clipper> clipper(m, "Clipper",  R"(
    The Clipper class manages the process of clipping polygons using a number of different Boolean operations,
    by providing a list of open or closed subject and clipping paths. These are internally represented with Int64
    precision, that requires the user to specify a scaleFactor. )"
    );


    /*
     * Path types are exported for convenience because these tend to be used often
     */
    py::enum_<Clipper2Lib::PathType>(m, "PathType",  "The path type")
        .value("Subject", Clipper2Lib::PathType::Subject, "The subject path")
        .value("Clip",    Clipper2Lib::PathType::Clip, "The clipping path")
        .export_values();

    /*
     * Boolean ops are exported for convenience because these tend to be used often
     */
    py::enum_<Clipper2Lib::ClipType>(m, "ClipType", py::arithmetic(), "The clipping operation type")
        .value("Union",        Clipper2Lib::ClipType::Union, "Union operation")
        .value("Difference",   Clipper2Lib::ClipType::Difference, "Difference operation")
        .value("Intersection", Clipper2Lib::ClipType::Intersection, "Intersection operation")
        .value("Xor",          Clipper2Lib::ClipType::Xor, "XOR operation")
        .export_values();

    py::enum_<Clipper2Lib::FillRule>(m, "FillRule", py::arithmetic(), "The fill rule to be used for the clipping operation")
        .value("EvenOdd",  Clipper2Lib::FillRule::EvenOdd, "Even and Odd Fill")
        .value("NonZero",  Clipper2Lib::FillRule::NonZero, "Non-Zero Fill")
        .value("Positive", Clipper2Lib::FillRule::Positive, "Positive Fill")
        .value("Negative", Clipper2Lib::FillRule::Negative, "Negative Fill");

    clipper.def(py::init<>())
        .def_property("scaleFactor", &pyclipr::Clipper::getScaleFactor, &pyclipr::Clipper::setScaleFactor, R"(
            The scale factor to be for transforming the input and output vectors. The default is 1000. )"
         )
        .def_property("preserveCollinear", &pyclipr::Clipper::getPreserveCollinear, &pyclipr::Clipper::setPreserveCollinear, R"(
             By default, when three or more vertices are collinear in input polygons (subject or clip),
             the Clipper object removes the 'inner' vertices before clipping. When enabled the PreserveCollinear property
             prevents this default behavior to allow these inner vertices to appear in the solution.
         )" )
        .def("addPath", &pyclipr::Clipper::addPath, py::arg("path"), py::arg("pathType"), py::arg("isOpen") = false, R"(
            The addPath method adds one or more closed subject paths (polygons) to the Clipper object.

            :param path: A list of 2D points (x,y) that define the path. Tuple or a numpy array may be provided
            :param pathType: A PathType enum value that indicates whether the path is a subject or a clip path.
            :param isOpen: A boolean value that indicates whether the path is closed or not. Default is 'False'
            :return: None
        )" )
        .def("addPaths", &pyclipr::Clipper::addPaths, py::arg("paths"), py::arg("pathType"), py::arg("isOpen") = false, R"(
            The AddPath method adds one or more closed subject paths (polygons) to the Clipper object.

            :param path: A list paths, each consisting 2D points (x,y) that define the path. A Tuple or a numpy array may be provided
            :param pathType: A PathType enum value that indicates whether the path is a subject or a clip path.
            :param isOpen: A boolean value that indicates whether the path is closed or not. Default is `False`
            :param returnZ: If `True`, returns a separate array of the Z attributes for clipped paths. Default is `False`
            :return: None
        )" )
        .def("execute", &pyclipr::Clipper::execute,  py::arg("clipType"),
                                                     py::arg_v("fillRule", Clipper2Lib::FillRule::EvenOdd, "FIllRule.EvenOdd"),
                                                     py::kw_only(), py::arg("returnOpenPaths") = false,
                                                     py::arg("returnZ") = false,
                                                     py::return_value_policy::take_ownership, R"(
            The execute method performs the Boolean clipping operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result. The default fillRule is
            even-odd typically used for the representation of polygons.

            :param clipType: The ClipType or the clipping operation to be used for the paths
            :param fillRule: A FillType enum value that indicates the fill representation for the paths
            :param returnOpenPaths: If `True`, returns a tuple consisting of both open and closed paths
            :param returnZ: If `True`, returns a separate array of the Z attributes for clipped paths. Default is `False`
            :return: A resultant paths that have been clipped )"
            )
        .def("execute2", &pyclipr::Clipper::execute2, py::arg("clipType"),
                                                      py::arg_v("fillRule", Clipper2Lib::FillRule::EvenOdd, "FIllRule.EvenOdd"),
                                                      py::kw_only(), py::arg("returnOpenPaths") = false,
                                                      py::arg("returnZ") = false,
                                                      py::return_value_policy::take_ownership, R"(
            The execute2 method performs the Boolean clipping operation on the polygons or paths that have been added
            to the clipper object. TThis method will return a PolyTree of the result structuring the output into the hierarchy of
            the paths that form the exterior and interior polygon.

            The default fillRule is even-odd typically used for the representation of polygons.

            :param clipType: The ClipType or the clipping operation to be used for the paths
            :param fillRule: A FillType enum value that indicates the fill representation for the paths
            :param returnOpenPaths: If `True`, returns a tuple consisting of both open and closed paths. Default is `False`
            :param returnZ: If `True`, returns a separate array of the Z attributes for clipped paths. Default is `False`
            :return: A resultant polytree of the clipped paths )"
            )

        .def("executeTree", &pyclipr::Clipper::execute2,  py::arg("clipType"),
                                                          py::arg_v("fillRule", Clipper2Lib::FillRule::EvenOdd, "FIllRule.EvenOdd"),
                                                          py::kw_only(), py::arg("returnOpenPaths") = false,
                                                          py::arg("returnZ") = false,
                                                          py::return_value_policy::take_ownership, R"(

        The `executeTree` method performs the Boolean clipping operation on the polygons or paths that have been added
        to the clipper object. TThis method will return a PolyTree of the result structuring the output into the hierarchy of
        the paths that form the exterior and interior polygon.

        The default `FillRule` is even-odd typically used for the representation of polygons.

        :param clipType: The ClipType or the clipping operation to be used for the paths
        :param fillRule: A FillType enum value that indicates the fill representation for the paths
        :param returnOpenPaths: If `True`, returns a tuple consisting of both open and closed paths. Default is `False`
        :param returnZ: If `True`, returns a separate array of the Z attributes for clipped paths. Default is `False`
        :return: A resultant paths that have been clipped )"
        )
        .def("clear", &pyclipr::Clipper::clear, R"(The clear method removes all the paths from the Clipper object.)" )
        .def("cleanUp", &pyclipr::Clipper::cleanUp);


    py::class_<pyclipr::ClipperOffset> clipperOffset(m, "ClipperOffset", R"(
    The ClipperOffset class manages the process of offsetting  (inflating/deflating)
    both open and closed paths using a number of different join types and end types.
    The library user will rarely need to access this unit directly since it will generally
    be easier to use the InflatePaths function when doing polygon offsetting.)"
     );

    py::enum_<Clipper2Lib::JoinType>(m, "JoinType", py::arithmetic(), "The join type to be used for the offsetting / inflation of paths")
        .value("Square", Clipper2Lib::JoinType::Square, "Square join type")
        .value("Round",  Clipper2Lib::JoinType::Round, "Round join type")
        .value("Miter",  Clipper2Lib::JoinType::Miter, "Miter join type");

    py::enum_<Clipper2Lib::EndType>(m, "EndType", py::arithmetic(), "The end type to be used for the offsetting / inflation of paths")
        .value("Square",  Clipper2Lib::EndType::Square, "Square end type")
        .value("Butt",    Clipper2Lib::EndType::Butt, "Butt end type")
        .value("Joined",  Clipper2Lib::EndType::Joined, "Joined end type")
        .value("Polygon", Clipper2Lib::EndType::Polygon, "Polygon end type")
        .value("Round",   Clipper2Lib::EndType::Round, "Round end type");

    clipperOffset.def(py::init<>())
        .def_property("scaleFactor", &pyclipr::ClipperOffset::getScaleFactor, &pyclipr::ClipperOffset::setScaleFactor,
            R"(Scale factor for transforming the input and output vectors. The default is 1000.)"
        )
        .def_property("arcTolerance", &pyclipr::ClipperOffset::getArcTolerance,   &pyclipr::ClipperOffset::setArcTolerance, R"(
            Firstly, this field/property is only relevant when JoinType = Round and/or EndType = Round.

            Since flattened paths can never perfectly represent arcs, this field/property specifies a maximum acceptable
            imprecision ('tolerance') when arcs are approximated in an offsetting operation. Smaller values will increase
            'smoothness' up to a point though at a cost of performance and in creating more vertices to construct the arc.

            The default ArcTolerance is 0.25 units. This means that the maximum distance the flattened path will deviate
            from the 'true' arc will be no more than 0.25 units (before rounding). )"  )
        .def_property("miterLimit", &pyclipr::ClipperOffset::getMiterLimit, &pyclipr::ClipperOffset::setMiterLimit,  R"(
             This property sets the maximum distance in multiples of delta that vertices can be offset from their original
             positions before squaring is applied. (Squaring truncates a miter by 'cutting it off' at 1 x delta distance
             from the original vertex.)

             The default value for MiterLimit is 2 (ie twice delta). )"
        )
        .def_property("preserveCollinear", &pyclipr::ClipperOffset::getPreserveCollinear, &pyclipr::ClipperOffset::setPreserveCollinear, R"(
             By default, when three or more vertices are collinear in input polygons (subject or clip),
             the Clipper object removes the 'inner' vertices before clipping. When enabled the `PreserveCollinear` property
             prevents this default behavior to allow these inner vertices to appear in the solution. )"
         )
        .def("addPath", &pyclipr::ClipperOffset::addPath, py::arg("path"),
                                                          py::arg("joinType"),
                                                          py::arg_v("endType", Clipper2Lib::EndType::Polygon, "Endtype.Polygon"),  R"(
            The addPath method adds one open or closed paths (polygon) to the ClipperOffset object.

            :param path: A list of 2D points (x,y) that define the path. Tuple or a numpy array may be provided for the path
            :param joinType: The JoinType to use for the offsetting / inflation of paths
            :param endType: The EndType to use for the offsetting / inflation of paths (default is Polygon)
            :return: None )"
        )
        .def("addPaths", &pyclipr::ClipperOffset::addPaths, py::arg("path"),
                                                            py::arg("joinType"),
                                                            py::arg_v("endType", Clipper2Lib::EndType::Polygon, "Endtype.Polygon"), R"(
            The addPath method adds one or more open / closed paths to the ClipperOffset object.

            :param path: A list of paths consisting of 2D points (x,y) that define the path. Tuple or a numpy array may be provided for each path
            :param joinType: The JoinType to use for the offsetting / inflation of paths
            :param endType: The EndType to use for the offsetting / inflation of paths
            :return: None)"
        )
        .def("execute", &pyclipr::ClipperOffset::execute,  py::arg("delta"), py::return_value_policy::take_ownership, R"(
            The `execute` method performs the offsetting/inflation operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result.

            :param delta: The offset to apply to the inflation/offsetting of paths and segments
            :return: The resultant offset paths
        )")
        .def("execute2", &pyclipr::ClipperOffset::execute2,  py::arg("delta"),  py::return_value_policy::take_ownership, R"(
            The `execute` method performs the offsetting/inflation operation on the polygons or paths that have been added
            to the clipper object. This method will return a PolyTree from the result, that considers the hierarchy of the interior and exterior
            paths of the polygon.

            :param delta: The offset to apply to the inflation/offsetting
            :return: A resultant offset paths created in a PolyTree64 Object )"
        )
        .def("executeTree", &pyclipr::ClipperOffset::execute2,  py::arg("delta"),  py::return_value_policy::take_ownership, R"(
            The `executeTree` method performs the offsetting/inflation operation on the polygons or paths that have been added
            to the clipper object. This method will return a PolyTree from the result, that considers the hierarchy of the interior and exterior
            paths of the polygon.

            :param delta: The offset to apply to the inflation/offsetting
            :return: A resultant offset paths created in a PolyTree64 Object )"
         )
        .def("clear", &pyclipr::ClipperOffset::clear, R"(The clear method removes all the paths from the ClipperOffset object.)");



#ifdef PROJECT_VERSION
    m.attr("__version__") = "PROJECT_VERSION";
#else
    m.attr("__version__") = "dev";
#endif

}

