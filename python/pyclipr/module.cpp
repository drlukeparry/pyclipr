#include <string>

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

#include <tuple>

using namespace Clipper2Lib;

namespace py = pybind11;

namespace pyclipr {

typedef Eigen::Matrix<uint64_t,Eigen::Dynamic,2> EigenVec2i;
typedef Eigen::Matrix<double,Eigen::Dynamic,2> EigenVec2d;

Clipper2Lib::Path64 simplifyPath(const Clipper2Lib::Path64 &path, double epsilon, bool isOpenPath = false)
{
    return Clipper2Lib::SimplifyPath(path, epsilon, isOpenPath);
}

Clipper2Lib::Paths64 simplifyPaths(const Clipper2Lib::Paths64 &paths, double epsilon, bool isOpenPath = false)
{
    return Clipper2Lib::SimplifyPaths(paths, epsilon, isOpenPath);
}

Paths64 polyTreeToPaths64(const Clipper2Lib::PolyTree64 &polytree)
{
    return Clipper2Lib::PolyTreeToPaths64(polytree);
}

void applyScaleFactor(const Clipper2Lib::PolyPath64 & polyPath, Clipper2Lib::PolyPath64 &newPath, uint64_t scaleFactor, bool invert = true) {
// Create a recursive copy of the structure

    for(uint64_t i = 0; i < polyPath.Count(); i++)
    {
        Clipper2Lib::Path64 path = polyPath[i]->Polygon();

        for (uint64_t j=0; j < path.size(); j++) {
            if (!invert) {
                path[j].x *= scaleFactor;
                path[j].y *= scaleFactor;

            } else {
                path[j].x /= scaleFactor;
                path[j].y /= scaleFactor;
            }
        }

        auto newChild = newPath.AddChild(path);

        applyScaleFactor(*(polyPath[i]), *newChild, scaleFactor);

    }
}


class Clipper : public Clipper2Lib::Clipper64 {

public:
// Write consturctor and destructor
    Clipper() : Clipper2Lib::Clipper64(), scaleFactor(1000.0) {}
    ~Clipper() {}

public:

    void addPath(const std::vector<std::tuple<uint64_t,uint64_t>> & path,
                 Clipper2Lib::PathType polyType, bool isOpen)
    {
       // Not used
        Clipper2Lib::Path64 p;

        for(auto pnt : path) {
            p.push_back(Clipper2Lib::Point64(std::get<0>(pnt), std::get<1>(pnt)));
        }

        this->AddPath(p, polyType, isOpen);
    }

    void addPathB(const EigenVec2d& path,
                 Clipper2Lib::PathType polyType, bool isOpen)
    {
        Clipper2Lib::Path64 p;
        p.reserve(path.rows());

        for(uint64_t i=0; i< path.rows(); i++) {
            p.push_back(Clipper2Lib::Point64(path(i,0) * scaleFactor, path(i,1) * scaleFactor));
        }

        this->AddPath(p, polyType, isOpen);
    }

    void addPaths(const std::vector<EigenVec2d> paths,
                  Clipper2Lib::PathType polyType, bool isOpen)
    {

        for(auto path : paths) {

            Clipper2Lib::Path64 p;
            p.reserve(path.rows());

            for(uint64_t i=0; i< path.rows(); i++) {

                p.push_back(Clipper2Lib::Point64(path(i,0) * scaleFactor, path(i,1) * scaleFactor));
            }

            this->AddPath(p, polyType, isOpen);
        }

    }

    void cleanUp() { this->CleanUp(); }
    void clear() { this->Clear(); }


    py::object execute(Clipper2Lib::ClipType clipType,  Clipper2Lib::FillRule fillRule, bool returnOpenPaths = false) {

        Paths64 closedPaths;
        Paths64 openPaths;

        this->Execute(clipType, fillRule, closedPaths, openPaths);

        std::vector<EigenVec2d> closedOut;

        for (auto &path : closedPaths) {

            EigenVec2d eigPath(path.size(), 2);

            for (uint64_t i=0; i<path.size(); i++) {
                eigPath(i,0) = path[i].x;
                eigPath(i,1) = path[i].y;
            }

            eigPath /= scaleFactor;
            closedOut.push_back(eigPath);
        }

        if(!returnOpenPaths) {
            return py::cast(closedOut);
        } else {

            std::vector<EigenVec2d> openOut;
            for (auto &path : openPaths) {

                EigenVec2d eigPath(path.size(), 2);

                for (uint64_t i=0; i<path.size(); i++) {
                    eigPath(i,0) = path[i].x;
                    eigPath(i,1) = path[i].y;
                }

                eigPath /= scaleFactor;
                openOut.push_back(eigPath);
            }

            return py::make_tuple(closedOut, openOut);
        }
    }

    py::object execute2(Clipper2Lib::ClipType clipType, Clipper2Lib::FillRule fillRule, bool returnOpenPaths = false) {

        Clipper2Lib::PolyTree64 polytree;
        Clipper2Lib::Paths64 openPaths;

        this->Execute(clipType, fillRule, polytree, openPaths);

        PolyTree64 *polytreeCpy = new PolyTree64();

        applyScaleFactor(polytree, *polytreeCpy, scaleFactor);

        if(returnOpenPaths) {
            std::vector<EigenVec2d> out2;
            for (auto &path : openPaths) {

                EigenVec2d eigPath(path.size(), 2);

                for (uint64_t i=0; i<path.size(); i++) {
                    eigPath(i,0) = path[i].x;
                    eigPath(i,1) = path[i].y;
                }

                eigPath /= scaleFactor;
                out2.push_back(eigPath);
            }

            return py::make_tuple(polytreeCpy, out2);
        } else {
            return  py::cast(polytreeCpy);
        }
    }

public:
    uint64_t scaleFactor;
};



class ClipperOffset : public Clipper2Lib::ClipperOffset {

public:
// Write consturctor and destructor
    ClipperOffset() : Clipper2Lib::ClipperOffset(), scaleFactor(1000.0) {}
    ~ClipperOffset() {}

public:

    void addPath(const std::vector<std::tuple<uint64_t,uint64_t>> & path,
                 Clipper2Lib::JoinType joinType, Clipper2Lib::EndType endType)
    {
       // Not used
        Clipper2Lib::Path64 p;

        for(auto pnt : path) {
            p.push_back(Clipper2Lib::Point64(std::get<0>(pnt), std::get<1>(pnt)));
        }

        this->AddPath(p, joinType, endType);
    }

    void addPathB(const EigenVec2d& path,
                 Clipper2Lib::JoinType joinType,
                 Clipper2Lib::EndType endType = Clipper2Lib::EndType::Polygon)
    {
        Clipper2Lib::Path64 p;
        p.reserve(path.rows());

        for(uint64_t i=0; i< path.rows(); i++) {
            p.push_back(Clipper2Lib::Point64(path(i,0) * scaleFactor, path(i,1) * scaleFactor));
        }

        this->AddPath(p, joinType, endType);
    }

    void addPaths(const std::vector<EigenVec2d> paths,
                  Clipper2Lib::JoinType joinType,
                  Clipper2Lib::EndType endType = Clipper2Lib::EndType::Polygon)
    {

        for(auto path : paths) {

            Clipper2Lib::Path64 p;
            p.reserve(path.rows());

            for(uint64_t i=0; i< path.rows(); i++) {

                p.push_back(Clipper2Lib::Point64(path(i,0) * scaleFactor, path(i,1) * scaleFactor));
            }

            this->AddPath(p, joinType, endType);
        }

    }

    void clear() { this->Clear(); }

    std::vector<EigenVec2d> execute(double delta) {

        Paths64 closedPaths;

        this->Execute(delta * scaleFactor, closedPaths);

        std::vector<EigenVec2d> closedOut;

        for (auto &path : closedPaths) {

            EigenVec2d eigPath(path.size(), 2);

            for (uint64_t i=0; i<path.size(); i++) {
                eigPath(i,0) = path[i].x;
                eigPath(i,1) = path[i].y;
            }

            eigPath /= scaleFactor;
            closedOut.push_back(eigPath);
        }


        return closedOut;

    }

    PolyTree64 * execute2(double delta) {

        Clipper2Lib::PolyTree64 polytree;

        this->Execute(delta * scaleFactor, polytree);

        PolyTree64 *polytreeCpy = new PolyTree64();

        applyScaleFactor(polytree, *polytreeCpy, scaleFactor);

        return polytreeCpy;
    }

public:
    uint64_t scaleFactor;
};

} // end of namespace pyclipr

PYBIND11_MODULE(pyclipr, m) {

    m.doc() = R"pbdoc(
        PyClipr Module
        -----------------------
        .. currentmodule:: pyclipr
        .. autosummary::
           :toctree: _generate

    )pbdoc";


    /*
     * Specify enums first
     */

    py::enum_<Clipper2Lib::PathType>(m, "PathType")
    .value("Subject", Clipper2Lib::PathType::Subject)
    .value("Clip",    Clipper2Lib::PathType::Clip)
    .export_values();

    py::enum_<Clipper2Lib::ClipType>(m, "ClipType")
    .value("Union",        Clipper2Lib::ClipType::Union)
    .value("Difference",   Clipper2Lib::ClipType::Difference)
    .value("Intersection", Clipper2Lib::ClipType::Intersection)
    .value("Xor",          Clipper2Lib::ClipType::Xor)
    .export_values();

    py::enum_<Clipper2Lib::FillRule>(m, "FillType")
    .value("EvenOdd",  Clipper2Lib::FillRule::EvenOdd)
    .value("NonZero",  Clipper2Lib::FillRule::NonZero)
    .value("Positive", Clipper2Lib::FillRule::Positive)
    .value("Negative", Clipper2Lib::FillRule::Negative)
    .export_values();

    py::enum_<Clipper2Lib::JoinType>(m, "JoinType")
    .value("Square", Clipper2Lib::JoinType::Square)
    .value("Round",  Clipper2Lib::JoinType::Round)
    .value("Miter",  Clipper2Lib::JoinType::Miter)
    .export_values();

    py::enum_<Clipper2Lib::EndType>(m, "EndType")
    .value("Square",  Clipper2Lib::EndType::Square)
    .value("Butt",    Clipper2Lib::EndType::Butt)
    .value("Joined",  Clipper2Lib::EndType::Joined)
    .value("Polygon", Clipper2Lib::EndType::Polygon)
    .value("Round",   Clipper2Lib::EndType::Round)
    .export_values();

    py::class_<Clipper2Lib::PolyPath>(m, "PolyPath")
        /*.def(py::init<>()) */
        .def_property_readonly("level", &Clipper2Lib::PolyPath::Level)
        .def_property_readonly("parent", &Clipper2Lib::PolyPath::Parent);

    py::class_<Clipper2Lib::PolyPath64>(m, "PolyTree")
        /* .def(py::init<>()) */
        .def_property_readonly("isHole", &Clipper2Lib::PolyPath64::IsHole)
        .def_property_readonly("area",   &Clipper2Lib::PolyPath64::Area)
        .def_property_readonly("polygon", [](const Clipper2Lib::PolyPath64 &s ) {
            Clipper2Lib::Path64 path = s.Polygon();

            pyclipr::EigenVec2d eigPath(path.size(), 2);

            for (uint64_t i=0; i<path.size(); i++) {
                eigPath(i,0) = path[i].x;
                eigPath(i,1) = path[i].y;
            }
            return eigPath;
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


    m.def("polyTreeToPaths64", &pyclipr::polyTreeToPaths64, py::return_value_policy::automatic

    )
    .def("simplifyPath", &pyclipr::simplifyPath, py::arg("path"), py::arg("epsilon"), py::arg("isOpenPath") = false,
                        py::return_value_policy::automatic, R"(
            This function removes vertices that are less than the specified epsilon distance from an imaginary line
            that passes through its 2 adjacent vertices. Logically, smaller epsilon values will be less aggressive
            in removing vertices than larger epsilon values. )"
     )
    .def("simplifyPaths", &pyclipr::simplifyPaths, py::arg("paths"), py::arg("epsilon"), py::arg("isOpenPath") = false,
                        py::return_value_policy::automatic, R"(
            This function removes vertices that are less than the specified epsilon distance from an imaginary line
            that passes through its 2 adjacent vertices. Logically, smaller epsilon values will be less aggressive
            in removing vertices than larger epsilon values. )"
    );


    py::class_<pyclipr::Clipper>(m, "Clipper")
        .def(py::init<>())
        .def_readwrite("scaleFactor", &pyclipr::Clipper::scaleFactor, R"(
            Scale factor to be for transforming the input and output vectors. The default is 1000 )"
         )
        .def_readwrite("preserveCollinear", &pyclipr::Clipper::PreserveCollinear, R"(
             By default, when three or more vertices are collinear in input polygons (subject or clip),
             the Clipper object removes the 'inner' vertices before clipping. When enabled the PreserveCollinear property
             prevents this default behavior to allow these inner vertices to appear in the solution.
         )" )
        //.def("addPath", &pyclipr::Clipper::addPath)
        .def("addPath", &pyclipr::Clipper::addPathB, py::arg("path"), py::arg("pathType"), py::arg("isClosed") = true, R"(
            The addPath method adds one or more closed subject paths (polygons) to the Clipper object.

            :param path: A list of 2D points (x,y) that define the path. Tuple or a numpy array may be provided
            :param pathType: A PathType enum value that indicates whether the path is a subject or a clip path.
            :param isClosed: A boolean value that indicates whether the path is closed or not. Default is 'True'
            :return: None
        )" )

        .def("addPaths", &pyclipr::Clipper::addPaths, py::arg("paths"), py::arg("pathType"), py::arg("isClosed") = true, R"(
            The AddPath method adds one or more closed subject paths (polygons) to the Clipper object.

            :param path: A list paths, each consisting 2D points (x,y) that define the path. A Tuple or a numpy array may be provided
            :param pathType: A PathType enum value that indicates whether the path is a subject or a clip path.
            :param isClosed: A boolean value that indicates whether the path is closed or not. Default is 'True'
            :return: None
        )" )
        .def("execute", &pyclipr::Clipper::execute,
                          py::arg("clipType"),
                          py::arg("fillType") =  Clipper2Lib::FillRule::EvenOdd,
                          py::kw_only(), py::arg("returnOpenPaths") = false,
                          py::return_value_policy::take_ownership, R"(
            The execute method performs the Boolean clipping operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result. The default fillType is
            even-odd typically used for the representation of polygons.

            :param clipType: The ClipType or the clipping operation to be used for the paths
            :param fillType: A FillType enum value that indicates the fill representation for the paths
            :param returnOpenPaths: If `True`, returns a tuple consisting of both open and closed paths
            :return: A resultant paths that have been clipped )"
            )
        .def("execute2", &pyclipr::Clipper::execute2,
                          py::arg("clipType"),
                          py::arg("fillType") =  Clipper2Lib::FillRule::EvenOdd,
                          py::kw_only(), py::arg("returnOpenPaths") = false,
                          py::return_value_policy::take_ownership, R"(
            The execute method performs the Boolean clipping operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result. The default fillType is
            even-odd typically used for the representation of polygons.

            :param clipType: The ClipType or the clipping operation to be used for the paths
            :param fillType: A FillType enum value that indicates the fill representation for the paths
            :param returnOpenPaths: If `True`, returns a tuple consisting of both open and closed paths
            :return: A resultant paths that have been clipped )"
            )
        .def("clear", &pyclipr::Clipper::clear, R"(The clear method removes all the paths from the Clipper object.)" )
        .def("cleanUp", &pyclipr::Clipper::cleanUp);



    py::class_<pyclipr::ClipperOffset>(m, "ClipperOffset")
        .def(py::init<>())
        .def_readwrite("scaleFactor", &pyclipr::ClipperOffset::scaleFactor, R"(
            Scale factor to be for transforming the input and output vectors. The default is 1000 )"
         )
        .def_property("arcTolerance", [](const Clipper2Lib::ClipperOffset &s ) { return s.ArcTolerance();}
                                    , [](Clipper2Lib::ClipperOffset &s, double val ) { return s.ArcTolerance(val);}, R"(
            Firstly, this field/property is only relevant when JoinType = Round and/or EndType = Round.

            Since flattened paths can never perfectly represent arcs, this field/property specifies a maximum acceptable
            imprecision ('tolerance') when arcs are approximated in an offsetting operation. Smaller values will increase
            'smoothness' up to a point though at a cost of performance and in creating more vertices to construct the arc.

            The default ArcTolerance is 0.25 units. This means that the maximum distance the flattened path will deviate
            from the 'true' arc will be no more than 0.25 units (before rounding). )"
        )
        .def_property("miterLimit", [](const Clipper2Lib::ClipperOffset &s ) { return s.MiterLimit();}
                                  , []( Clipper2Lib::ClipperOffset &s, double val ) { return s.MiterLimit(val);}, R"(
             This property sets the maximum distance in multiples of delta that vertices can be offset from their original
             positions before squaring is applied. (Squaring truncates a miter by 'cutting it off' at 1 x delta distance
             from the original vertex.)

             The default value for MiterLimit is 2 (ie twice delta). )"
        )
        .def_property("preserveCollinear", [](const Clipper2Lib::ClipperOffset &s ) { return s.PreserveCollinear();}
                                         , [](Clipper2Lib::ClipperOffset &s, bool val ) { return s.PreserveCollinear(val);}, R"(
             By default, when three or more vertices are collinear in input polygons (subject or clip),
             the Clipper object removes the 'inner' vertices before clipping. When enabled the PreserveCollinear property
             prevents this default behavior to allow these inner vertices to appear in the solution. )"
         )
        //.def("addPath", &pyclipr::Clipper::addPath)
        .def("addPath", &pyclipr::ClipperOffset::addPathB, py::arg("path"),
                         py::arg("joinType"),
                         py::arg("endType") = Clipper2Lib::EndType::Polygon, R"(
            The addPath method adds one open / closed   paths (polygons) to the ClipperOffset object.

            :param path: A list of 2D points (x,y) that define the path. Tuple or a numpy array may be provided for the path
            :param joinType: The JoinType to use for the offseting / inflation of paths
            :param endType: The EndType to use for the offseting / inflation of paths (default is Polygon)
            :return: None )"
        )

        .def("addPaths", &pyclipr::ClipperOffset::addPaths, py::arg("path"),
                          py::arg("joinType"),
                          py::arg("endType") = Clipper2Lib::EndType::Polygon, R"(
            The addPath method adds one or more open / closed paths to the ClipperOffset object.

            :param path: A list of paths conisting of 2D points (x,y) that define the path. Tuple or a numpy array may be provided for each path
            :param joinType: The JoinType to use for the offseting / inflation of paths
            :param endType: The EndType to use for the offseting / inflation of paths
            :return: None)"
        )
        .def("execute", &pyclipr::ClipperOffset::execute,
                          py::arg("delta"),
                          py::return_value_policy::take_ownership, R"(
            The execute method performs the offseting/inflation operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result.

            :param delta: The offset to apply to the inflation/offseting
            :return: A resultant offset paths
        )")
        .def("execute2", &pyclipr::ClipperOffset::execute2,
                          py::arg("delta"),
                          py::return_value_policy::take_ownership, R"(
            The execute method performs the offseting/inflation operation on the polygons or paths that have been added
            to the clipper object. This method will return a list of paths from the result.

            :param delta: The offset to apply to the inflation/ofsetting
            :return: A resultant offset paths created in a PolyTree64 Object )"
        )
        .def("clear", &pyclipr::ClipperOffset::clear, R"(The clear method removes all the paths from the ClipperOffset object.)");

#ifdef PROJECT_VERSION
    m.attr("__version__") = "PROJECT_VERSION";
#else
    m.attr("__version__") = "dev";
#endif

}

