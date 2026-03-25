#include "svg_export.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

// --- BDD SVG export ---

static std::string bdd_svg_generate(bddp f, const SvgParams& params) {
    (void)f; (void)params;
    throw std::logic_error("bdd_save_svg: not yet implemented");
}

void bdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = bdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("bdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void bdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << bdd_svg_generate(f, params);
}

std::string bdd_save_svg(bddp f, const SvgParams& params) {
    return bdd_svg_generate(f, params);
}

// --- ZDD SVG export ---

static std::string zdd_svg_generate(bddp f, const SvgParams& params) {
    (void)f; (void)params;
    throw std::logic_error("zdd_save_svg: not yet implemented");
}

void zdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = zdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("zdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void zdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << zdd_svg_generate(f, params);
}

std::string zdd_save_svg(bddp f, const SvgParams& params) {
    return zdd_svg_generate(f, params);
}

// --- QDD SVG export ---

static std::string qdd_svg_generate(bddp f, const SvgParams& params) {
    (void)f; (void)params;
    throw std::logic_error("qdd_save_svg: not yet implemented");
}

void qdd_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = qdd_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("qdd_save_svg: cannot open ") + filename);
    ofs << svg;
}

void qdd_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << qdd_svg_generate(f, params);
}

std::string qdd_save_svg(bddp f, const SvgParams& params) {
    return qdd_svg_generate(f, params);
}

// --- UnreducedDD SVG export ---

static std::string unreduced_svg_generate(bddp f, const SvgParams& params) {
    (void)f; (void)params;
    throw std::logic_error("unreduced_save_svg: not yet implemented");
}

void unreduced_save_svg(const char* filename, bddp f, const SvgParams& params) {
    std::string svg = unreduced_svg_generate(f, params);
    std::ofstream ofs(filename);
    if (!ofs)
        throw std::runtime_error(std::string("unreduced_save_svg: cannot open ") + filename);
    ofs << svg;
}

void unreduced_save_svg(std::ostream& strm, bddp f, const SvgParams& params) {
    strm << unreduced_svg_generate(f, params);
}

std::string unreduced_save_svg(bddp f, const SvgParams& params) {
    return unreduced_svg_generate(f, params);
}
