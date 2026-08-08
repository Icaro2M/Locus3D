/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/manufacturing/pipeline/AnalysisPipeline.h"

#include "kernel/manufacturing/analyzers/geometry/DegenerateGeometryAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/MinimumFeatureSizeAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/SelfIntersectionAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/VolumeAnalyzer.h"
#include "kernel/manufacturing/analyzers/process/OverhangAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallAnalyzerFactory.h"
#include "kernel/manufacturing/analyzers/topology/IslandAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/ManifoldAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/NormalConsistencyAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/OrientationAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/WatertightAnalyzer.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/core/PrintIssueType.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"
#include "kernel/manufacturing/mesh/AnalysisMeshBuilder.h"
#include "kernel/manufacturing/profiles/PrintProfile.h"

#include <memory>

namespace locus::kernel::manufacturing {

    AnalysisReport AnalysisPipeline::analyze(
        const geometry::LEM& mesh,
        const AnalysisOptions& options)
    {
        AnalysisReport report;

        execute(
            mesh,
            nullptr,
            report,
            options);

        return report;
    }

    AnalysisReport AnalysisPipeline::analyze(
        const geometry::LEM& mesh,
        const PrintProfile& profile,
        const AnalysisOptions& options)
    {
        AnalysisReport report;

        execute(
            mesh,
            &profile,
            report,
            options);

        return report;
    }

    void AnalysisPipeline::analyze_into(
        const geometry::LEM& mesh,
        AnalysisReport& report,
        const AnalysisOptions& options)
    {
        execute(
            mesh,
            nullptr,
            report,
            options);
    }

    void AnalysisPipeline::analyze_into(
        const geometry::LEM& mesh,
        const PrintProfile& profile,
        AnalysisReport& report,
        const AnalysisOptions& options)
    {
        execute(
            mesh,
            &profile,
            report,
            options);
    }

    void AnalysisPipeline::execute(
        const geometry::LEM& mesh,
        const PrintProfile* profile,
        AnalysisReport& report,
        const AnalysisOptions& options)
    {
        /*
         * A pipeline execution represents one complete snapshot rather than an
         * incremental append operation.
         */
        report.clear();

        /*
         * Build once and share across every analyzer.
         *
         * This guarantees that orientation, intersections, volume, thin-wall,
         * and overhang all observe exactly the same triangulation.
         */
        AnalysisMesh analysisMesh =
            AnalysisMeshBuilder::build(mesh);

        report.metrics().analysisTriangleCount =
            analysisMesh.triangle_count();

        report.metrics().bounds =
            analysisMesh.bounds();

        AnalysisContext context;
        context.mesh = &mesh;
        context.profile = profile;
        context.analysisMesh = &analysisMesh;

        //=====================================================================
        // Topology
        //=====================================================================

        if (options.analyzeManifold) {
            ManifoldAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        if (options.analyzeWatertight) {
            WatertightAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        if (options.analyzeNormalConsistency) {
            NormalConsistencyAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        /*
         * Global orientation is meaningful only after the local prerequisites
         * have actually been evaluated and found acceptable.
         *
         * Absence of an issue is not treated as proof when its prerequisite
         * analyzer was explicitly disabled.
         */
        const bool orientationPrerequisitesEvaluated =
            options.analyzeManifold &&
            options.analyzeWatertight &&
            options.analyzeNormalConsistency;

        const bool orientationBlocked =
            report.has_issue_type(
                PrintIssueType::NonManifoldEdge) ||
            report.has_issue_type(
                PrintIssueType::OpenBoundary) ||
            report.has_issue_type(
                PrintIssueType::InconsistentNormals);

        if (options.analyzeOrientation &&
            orientationPrerequisitesEvaluated &&
            !orientationBlocked) {

            OrientationAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        if (options.analyzeIslands) {
            IslandAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        //=====================================================================
        // Geometry
        //=====================================================================

        if (options.analyzeDegenerateGeometry) {
            DegenerateGeometryAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        if (options.analyzeSelfIntersection) {
            SelfIntersectionAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        if (options.analyzeMinimumFeatureSize &&
            profile != nullptr) {

            MinimumFeatureSizeAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        /*
         * Physical volume is intentionally conservative.
         *
         * VolumeAnalyzer itself validates closure, but the pipeline additionally
         * requires the analyses capable of detecting ambiguous surface winding
         * and self-intersection to have executed successfully.
         */
        const bool volumePrerequisitesEvaluated =
            options.analyzeManifold &&
            options.analyzeWatertight &&
            options.analyzeNormalConsistency &&
            options.analyzeDegenerateGeometry &&
            options.analyzeSelfIntersection;

        const bool volumeBlocked =
            report.has_issue_type(
                PrintIssueType::NonManifoldEdge) ||
            report.has_issue_type(
                PrintIssueType::OpenBoundary) ||
            report.has_issue_type(
                PrintIssueType::InconsistentNormals) ||
            report.has_issue_type(
                PrintIssueType::DegenerateGeometry) ||
            report.has_issue_type(
                PrintIssueType::SelfIntersection);

        if (options.analyzeVolume &&
            volumePrerequisitesEvaluated &&
            !volumeBlocked) {

            VolumeAnalyzer analyzer;
            analyzer.analyze(context, report);
        }

        //=====================================================================
        // Thin wall
        //=====================================================================

        if (options.analyzeThinWall &&
            profile != nullptr) {

            std::unique_ptr<IThinWallAnalyzer> analyzer =
                ThinWallAnalyzerFactory::create(
                    options.thinWallQuality);

            if (analyzer != nullptr) {
                analyzer->analyze(
                    context,
                    report);
            }
        }

        //=====================================================================
        // Process
        //=====================================================================

        if (options.analyzeOverhang &&
            profile != nullptr) {

            OverhangAnalyzer analyzer{
                options.buildDirection
            };

            analyzer.analyze(
                context,
                report);
        }
    }

}