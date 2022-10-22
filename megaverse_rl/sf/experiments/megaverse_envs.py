from sample_factory.runner.run_description import Experiment, ParamGrid, RunDescription

_params = ParamGrid(
    [
        ("seed", [00, 11, 22, 33]),
        (
            "env",
            [
                'TowerBuilding',
                # 'ObstaclesEasy',
                # 'ObstaclesHard',
                # 'Collect',
                # 'Sokoban',
                # 'HexMemory',
                # 'HexExplore',
                # 'Rearrange',
            ],
        ),
    ]
)

_experiments = [
    Experiment(
        "megaverse_envs",
        "python -m megaverse_rl.sf.train_megaverse --train_for_seconds=360000000 --train_for_env_steps=2000000000 --algo=APPO --gamma=0.997 --use_rnn=True --rnn_num_layers=2 --num_workers=12 --num_envs_per_worker=2 --num_epochs=1 --rollout=32 --recurrence=32 --batch_size=4096 --actor_worker_gpus 0 --env_gpu_observations=False --num_policies=1 --with_pbt=False --max_grad_norm=0.0 --exploration_loss=symmetric_kl --exploration_loss_coeff=0.001 --megaverse_num_simulation_threads=1 --megaverse_num_envs_per_instance=32 --megaverse_num_agents_per_env=1 --megaverse_use_vulkan=True --policy_workers_per_policy=2 --reward_clip=30 --env=TowerBuilding --experiment=TowerBuilding_run0 --with_wandb=True --wandb_project=megaverse-benchmark --wandb_group=megaverse --wandb_tags run0",
        _params.generate_params(randomize=False),
    ),
]


RUN_DESCRIPTION = RunDescription("megaverse_envs", experiments=_experiments)
# python -m sample_factory.runner.run --run=megaverse_rl.sf.experiments.megaverse_envs --runner=processes --max_parallel=8  --pause_between=1 --experiments_per_gpu=10000 --num_gpus=1
