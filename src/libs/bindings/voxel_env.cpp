#include <utility>

#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <util/tiny_logger.hpp>

#include <env/env.hpp>
#include <v4r_rendering/v4r_env_renderer.hpp>
#include <magnum_rendering/magnum_env_renderer.hpp>

namespace py = pybind11;



void setVoxelEnvLogLevel(int level)
{
    setLogLevel(LogLevel(level));
}


class VoxelEnvGym
{
public:
    VoxelEnvGym(
        int w, int h,
        int numEnvs, int numAgentsPerEnv, int numSimulationThreads,
        float verticalLookLimit,
        bool useVulkan,
        std::map<std::string, float> floatParams
    )
    : numEnvs{numEnvs}
    , numAgentsPerEnv{numAgentsPerEnv}
    , useVulkan{useVulkan}
    , w{w}
    , h{h}
    , numSimulationThreads{numSimulationThreads}
    {
        for (int i = 0; i < numEnvs; ++i)
            envs.emplace_back(std::make_unique<Env>(numAgentsPerEnv, verticalLookLimit, floatParams));

        rewards = std::vector<float>(size_t(numEnvs * numAgentsPerEnv));
    }

    void setFloatParams()
    {

    }

    void seed(int seedValue)
    {
        TLOG(INFO) << "Seeding vector env with seed value " << seedValue;

        rng.seed((unsigned long)seedValue);
        for (auto &e : envs) {
            const auto noise = randRange(0, 1 << 30, rng);
            e->seed(noise);
        }
    }

    int numAgents() const
    {
        return envs.front()->getNumAgents();
    }

    void reset()
    {
        if (!vectorEnv) {
            if (useVulkan)
                renderer = std::make_unique<V4REnvRenderer>(envs, w, h);
            else
                renderer = std::make_unique<MagnumEnvRenderer>(envs, w, h);

            vectorEnv = std::make_unique<VectorEnv>(envs, *renderer, numSimulationThreads);
        }

        // this also resets the main renderer
        vectorEnv->reset();

        if (hiresRenderer)
            for (int envIdx = 0; envIdx < int(envs.size()); ++envIdx)
                hiresRenderer->reset(*envs[envIdx], envIdx);
    }

    std::vector<int> actionSpaceSizes()
    {
        return Env::actionSpaceSizes;
    }

    void setActions(int envIdx, int agentIdx, std::vector<int> actions)
    {
        int actionIdx = 0, actionMask = 0;
        const auto &spaces = Env::actionSpaceSizes;

        for (int i = 0; i < int(actions.size()); ++i) {
            const auto action = actions[i];

            if (action > 0)
                actionMask = actionMask | (1 << (actionIdx + action));

            const auto numNonIdleActions = spaces[i] - 1;
            actionIdx += numNonIdleActions;
        }

        envs[envIdx]->setAction(agentIdx, Action(actionMask));
    }

    void step()
    {
        vectorEnv->step();
    }

    bool isDone(int envIdx)
    {
        return vectorEnv->done[envIdx];
    }

    std::vector<float> getLastRewards()
    {
        int i = 0;

        for (int envIdx = 0; envIdx < numEnvs; ++envIdx)
            for (int agentIdx = 0; agentIdx < numAgentsPerEnv; ++agentIdx)
                rewards[i++] = envs[envIdx]->getLastReward(agentIdx);

        return rewards;
    }

    py::array_t<uint8_t> getObservation(int envIdx, int agentIdx)
    {
        const uint8_t *obsData = renderer->getObservation(envIdx, agentIdx);
        return py::array_t<uint8_t>({h, w, 4}, obsData, py::none{});  // numpy object does not own memory
    }

    /**
     * Call this before the first call to render()
     */
    void setRenderResolution(int hiresW, int hiresH)
    {
        renderW = hiresW;
        renderH = hiresH;
    }

    void drawHires()
    {
        if (!hiresRenderer) {
            if (useVulkan)
                hiresRenderer = std::make_unique<V4REnvRenderer>(envs, renderW, renderH);
            else
                hiresRenderer = std::make_unique<MagnumEnvRenderer>(envs, renderW, renderH);

            for (int envIdx = 0; envIdx < int(envs.size()); ++envIdx)
                hiresRenderer->reset(*envs[envIdx], envIdx);
        }

        for (int envIdx = 0; envIdx < int(envs.size()); ++envIdx)
            hiresRenderer->preDraw(*envs[envIdx], envIdx);
        hiresRenderer->draw(envs);
        for (int envIdx = 0; envIdx < int(envs.size()); ++envIdx)
            hiresRenderer->postDraw(*envs[envIdx], envIdx);
    }

    py::array_t<uint8_t> getHiresObservation(int envIdx, int agentIdx)
    {
        const uint8_t *obsData = hiresRenderer->getObservation(envIdx, agentIdx);
        return py::array_t<uint8_t>({renderH, renderW, 4}, obsData, py::none{});  // numpy object does not own memory
    }

    float trueObjective(int envIdx) const
    {
        return vectorEnv->trueObjectives[envIdx];
    }

    std::map<std::string, float> getRewardShaping(int envIdx, int agentIdx)
    {
        return envs[envIdx]->rewardShaping[agentIdx];
    }

    void setRewardShaping(int envIdx, int agentIdx, std::map<std::string, float> rewardShaping)
    {
        envs[envIdx]->rewardShaping[agentIdx] = std::move(rewardShaping);
    }

    /**
     * Explicitly destroy the env and the renderer to avoid doing this when the Python object goes out-of-scope.
     */
    void close()
    {
        if (vectorEnv)
            vectorEnv->close();

        hiresRenderer.reset();
        renderer.reset();
        vectorEnv.reset();

        envs.clear();
    }

private:
    Envs envs;
    int numEnvs, numAgentsPerEnv;
    std::vector<float> rewards;  // to avoid reallocating on every call

    std::unique_ptr<VectorEnv> vectorEnv;
    std::unique_ptr<EnvRenderer> renderer, hiresRenderer;

    Rng rng{std::random_device{}()};

    bool useVulkan;
    int w, h;
    int renderW = 768, renderH = 432;

    int numSimulationThreads;
};



PYBIND11_MODULE(voxel_env, m)
{
    m.doc() = "voxel env"; // optional module docstring

    m.def("set_voxel_env_log_level", &setVoxelEnvLogLevel, "Voxel Env Log Level (0 to disable all logs, 2 for warnings");

    py::class_<VoxelEnvGym>(m, "VoxelEnvGym")
        .def(py::init<int, int, int, int, int, float, bool, std::map<std::string, float>>())
        .def("num_agents", &VoxelEnvGym::numAgents)
        .def("action_space_sizes", &VoxelEnvGym::actionSpaceSizes)
        .def("seed", &VoxelEnvGym::seed)
        .def("reset", &VoxelEnvGym::reset)
        .def("set_actions", &VoxelEnvGym::setActions)
        .def("step", &VoxelEnvGym::step)
        .def("is_done", &VoxelEnvGym::isDone)
        .def("get_observation", &VoxelEnvGym::getObservation)
        .def("get_last_rewards", &VoxelEnvGym::getLastRewards)
        .def("true_objective", &VoxelEnvGym::trueObjective)
        .def("set_render_resolution", &VoxelEnvGym::setRenderResolution)
        .def("draw_hires", &VoxelEnvGym::drawHires)
        .def("get_hires_observation", &VoxelEnvGym::getHiresObservation)
        .def("get_reward_shaping", &VoxelEnvGym::getRewardShaping)
        .def("set_reward_shaping", &VoxelEnvGym::setRewardShaping)
        .def("close", &VoxelEnvGym::close);
}