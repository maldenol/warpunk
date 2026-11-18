# Warpunk

Warpunk is a multiplayer real-time strategy in Unreal Engine, and a demonstration of the systems I designed for it: a pooling lifecycle shared by actors, components, and plain C++ objects; data-driven entity setup; a custom movement component with its own prediction and replication; an optimized projectile replication and prediction path; and team-based, dormancy-based fog of war.

## Pooling lifecycle

Every reusable object implements `IWpPooledObjectInterface`. The interface owns the workflow. `OnPooledObjectAcquiredOrReleased` is the hook each object fills in.

**Acquire.** `AWpActorPool::Acquire` places the actor and calls `AcquirePooledObject` with a typed payload (`FWpPooledEntityData`, `FWpPooledProjectileData`, `FWpPooledWeaponData`). The call does two things, in order:

1. `OnPooledObjectAcquiredOrReleased(true, payload)` on the object itself. The actor reads the payload and configures collision, mesh, movement coefficients, team, and weapons.
2. The same `AcquirePooledObject` on every object returned by `GetInnerPooledObjects`, with the same payload. An entity forwards to belligerent, endurance, intel, movement, orders, production, and weapons. Each of those configures itself and, where it owns further objects, acquires them too. `UWpWeaponComponent` builds up to four `FWpWeapon` instances in a `TVariant` and acquires each one. A component whose config block is absent deactivates itself: a building does not start the movement component, a unit without intel does not start the vision volume.

**Release** runs the tree in the opposite order. Inner objects release first, then the owner runs `OnPooledObjectAcquiredOrReleased(false, {})`. Authority turns collision off, hides the actor, and returns it to the free set at the origin. The actor stays spawned. The next acquire is another pass over the same tree with a new payload.

**Visibility** is a third walk, `SetPooledObjectVisibility`, used by the fog-of-war manager. It hides or shows the mesh and, on clients, starts or stops simulation for actors the local team can see.

**Who starts the walk.** The server starts it from the pool. Clients start the same walk from replication: `OnRep_EntityConfigId` and `OnRep_ProjectileConfig` rebuild the payload from the replicated id and call `AcquirePooledObject`. Initialization code is written once and runs on both sides. Authority-only work (enabling collision, resetting simulation state, spawning weapons) returns before it runs on a client.

Pools are per actor class per world. The match reserves commanders, entities, and projectiles up front. Each client reserves a pool of non-replicated `AWpLocalEntity` ghosts for last-known buildings and placement preview. When a pool is empty it grows by half and spawns the extra actors across later frames.

The same contract covers `UObject` actors and components and non-reflected objects. Weapons and order handlers live in `TVariant` storage, so changing what an entity is does not allocate a new `UObject`.

## Data-driven entities

`AWpEntity` is the only replicated pawn. A row in `DT_EntityConfigs` (`FWpEntityConfig`) decides what an acquire produces: building or unit, mesh, movement, endurance, and optional intel, production, orders, and up to four weapons.

Editing the table runs `OnDataTableChanged`, which validates the row and bakes the values the simulation needs:

- collision shape and mesh offset from the skeletal-mesh bounds
- acceleration coefficients solved so the unit reaches its top speed in the configured time on the configured slope
- braking, inertia, ground friction, and wheel angle from the turning radius
- weapon cooldown, ballistic speed from range and gravity (`v² = range · g`), and the projectile mesh transform

Acquire copies that row through the pooling tree. Optional blocks are `TOptional`, so a row simply omits the systems that entity does not have.

## Movement, prediction, and replication

Units move with `UWpEntityMovementComponent`, a pawn movement component written for this project. It simulates three locomotion models from the entity row: legs, tracks, and wheels. One tick integrates acceleration and braking, ground friction and slipping, step-up, penetration, wall and entity impacts, and wheel steer. The maximum wheel angle is the angle whose turning radius matches the config.

Navigation feeds that component. `UWpCrowdFollowingComponent` sizes the crowd agent from the collision extent and submits desired velocity through `RequestPathMove`. Inside the braking distance the input drops to zero and the movement component bleeds speed with its own braking curve.

Replication is part of the same component. Actor movement replication stays off for units. While the authority simulates, each moving tick pushes one `FWpEntityMovementComponentPredictionData` with push-model replication: server time, pose, input, linear and angular speed, wheel angle, and the target mesh offset. The client applies the pose delta, restores those speeds, and calls the same `TickMovement` for `serverNow - snapshotTime`. Client and server run one integrator, and the snapshot is the state that integrator needs to catch up.

The skeletal mesh is an interpolated component on a separate scene root from the collision. A correction moves the collision immediately and eases the mesh over a short smoothing window, so a snapshot does not pop the visual. The locally controlled commander sends input with `ServerSetCommanderMovementInput` only when the vector changes, and keeps simulating from that input on the owning client.

Buildings do not use this component. They replicate their transform with actor movement replication, because they are placed and then stay put.

## Projectile replication and prediction

Projectiles are pooled actors simulated by `UWpProjectileMovementComponent`. A shot acquires one with `FWpPooledProjectileData`: weapon config id, initial velocity, damage. Radius, mesh, and speed come from the same entity row. Impact applies damage and releases the actor back to the pool.

A projectile's whole network life is two pushes of `FWpProjectileMovementComponentPredictionData`, both push-model. Acquire writes the acquired flag, server time, pose, and velocity. Release writes the flag clear. There is no per-tick transform stream. The client snaps to the snapshot, copies the velocity, and ticks the same movement component across the time since `ServerTime`, which lands the projectile where the server already is. Ballistics are deterministic from that velocity, so one snapshot covers the flight. Releasing clears the acquired flag, and the client plays the impact and stops the component. The next shot reuses the actor and its channel: a new acquire is a new snapshot on the same object.

## Fog of war, per team, tied to dormancy

Sight is a physics volume. `UWpIntelComponent` is a static mesh scaled to the config range and drawn only into a scene capture. `AWpFogOfWarManager` captures the map orthographically into a render target and composites the fog material. On a client the volume is included in that capture when the unit belongs to the local team and is operational, so each player’s fog is painted only by their own vision.

On the server, overlap begin and end adjust an observer count on `AWpPooledActorVisibilityManager`, one replicated manager per team and relevant only to that team. Visibility changes travel as a `FFastArraySerializer` of network GUID plus a visible bit. The client applies the bit once the GUID resolves, then runs `SetPooledObjectVisibility` down the pooling tree.

That bit is the dormancy policy. Entities and projectiles use `DORM_DormantPartial` and stay relevant. `GetNetDormancy` reports the actor dormant for a viewer whose team cannot see it. `UWpActorChannel` accepts dormancy, and `UWpIpConnection::FlushDormancy` ignores a wake-up for an actor that team still cannot see. The channel stays quiet until a friendly vision volume sees the actor and the server flushes dormancy.

A building that drops out of sight leaves a last-known ghost: the client acquires an `AWpLocalEntity` at that transform, runs a short overlap pass (`RememberIntel`), and releases it. Placement preview uses the same ghost pool.

## Orders

An order is a compact struct: type, target location, target entity, or production ids. `UWpOrderReceiverComponent` queues orders and stores the active handler in a `TVariant`: move, patrol, attack, strike, assist, repair, reclaim, self-destruct, produce building, produce unit, and the production-proxy and infinite-production toggles. `UWpOrderSenderComponent` does marquee selection, groups the selection by default-order category, and sends the batch in one reliable RPC. Ground, ally, and enemy defaults come from `FWpDefaultOrders` on the entity row.

## License

Warpunk is released under the MIT License. Copyright 2024–2026 Denys Malovanyi. See [LICENSE](LICENSE).
