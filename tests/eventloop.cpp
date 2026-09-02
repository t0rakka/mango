/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <mango/window/window.hpp>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_continuous_default_schedules()
    {
        EventLoopState state;
        state.config.mode = FrameMode::Continuous;
        state.config.waitForFrame = true;
        state.config.maxFrameRate = 0.0;
        state.reset();

        CHECK(state.shouldScheduleFrame(1'000));
        CHECK(state.computeWaitTimeoutMs(1'000) == 0); // needs_redraw from reset

        return true;
    }

    bool test_wait_for_frame_blocks_while_in_flight()
    {
        EventLoopState state;
        state.config.mode = FrameMode::Continuous;
        state.config.waitForFrame = true;
        state.config.maxFrameRate = 0.0;
        state.config.pollTimeoutMs = 7;
        state.reset();
        state.needs_redraw = false;
        state.frame_in_flight = true;

        CHECK(!state.shouldScheduleFrame(5'000));
        CHECK(state.computeWaitTimeoutMs(5'000) == 7);

        state.frame_in_flight = false;
        CHECK(state.shouldScheduleFrame(5'000));

        return true;
    }

    bool test_wait_for_frame_disabled_ignores_in_flight()
    {
        EventLoopState state;
        state.config.mode = FrameMode::Continuous;
        state.config.waitForFrame = false;
        state.config.maxFrameRate = 0.0;
        state.reset();
        state.needs_redraw = false;
        state.frame_in_flight = true;

        CHECK(state.shouldScheduleFrame(2'000));

        return true;
    }

    bool test_ondemand_idle_until_invalidate_or_deadline()
    {
        EventLoopState state;
        state.config.mode = FrameMode::OnDemand;
        state.config.waitForFrame = true;
        state.reset();
        state.needs_redraw = false;

        CHECK(!state.shouldScheduleFrame(10'000));
        CHECK(state.computeWaitTimeoutMs(10'000) == EventLoopState::WAIT_INFINITE);

        state.invalidate();
        CHECK(state.shouldScheduleFrame(10'000));
        CHECK(state.computeWaitTimeoutMs(10'000) == 0);

        state.needs_redraw = false;
        state.next_frame_deadline_us = 20'000;
        CHECK(!state.shouldScheduleFrame(15'000));
        CHECK(state.computeWaitTimeoutMs(15'000) == 5);
        CHECK(state.shouldScheduleFrame(20'000));
        CHECK(state.computeWaitTimeoutMs(20'000) == 0);

        return true;
    }

    bool test_max_frame_rate_gate()
    {
        EventLoopState state;
        state.config.mode = FrameMode::Continuous;
        state.config.waitForFrame = true;
        state.config.maxFrameRate = 100.0; // 10 ms interval
        state.reset();
        state.needs_redraw = false;
        state.last_frame_time_us = 1'000'000;

        CHECK(!state.shouldScheduleFrame(1'005'000)); // +5 ms
        CHECK(state.shouldScheduleFrame(1'010'000)); // +10 ms

        return true;
    }

    bool test_consume_invalidated()
    {
        EventLoopState state;
        state.reset();

        CHECK(state.consumeInvalidated());
        CHECK(!state.needs_redraw);
        CHECK(!state.consumeInvalidated());

        state.invalidate();
        CHECK(state.consumeInvalidated());
        CHECK(!state.needs_redraw);

        return true;
    }

    bool test_compute_dt()
    {
        EventLoopState state;
        state.reset();

        CHECK(state.computeDt(1'000'000) == 0.0);
        state.last_frame_time_us = 1'000'000;
        CHECK(state.computeDt(1'500'000) == 0.5);

        return true;
    }

    const Case cases[] =
    {
        { "continuous_default_schedules", test_continuous_default_schedules },
        { "wait_for_frame_blocks_while_in_flight", test_wait_for_frame_blocks_while_in_flight },
        { "wait_for_frame_disabled_ignores_in_flight", test_wait_for_frame_disabled_ignores_in_flight },
        { "ondemand_idle_until_invalidate_or_deadline", test_ondemand_idle_until_invalidate_or_deadline },
        { "max_frame_rate_gate", test_max_frame_rate_gate },
        { "consume_invalidated", test_consume_invalidated },
        { "compute_dt", test_compute_dt },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("eventloop", cases, std::size(cases), argc, argv);
}
