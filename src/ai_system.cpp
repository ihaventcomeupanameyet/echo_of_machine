// internal
// robot ai will be shifted here
#include "ai_system.hpp"

vec2 AISystem::calculateSeparation(Entity entity) {
    Motion& entity_motion = registry.motions.get(entity);
    Boid& boid = registry.boids.get(entity);
    vec2 steering(0, 0);
    int count = 0;

    for (Entity other : registry.boids.entities) {
        if (other.id != entity.id) {
            Motion& other_motion = registry.motions.get(other);
            float d = length(other_motion.position - entity_motion.position);

            if (d > 0 && d < boid.avoid_radius.x) {
                vec2 diff = normalize(entity_motion.position - other_motion.position);
                diff /= d;
                steering += diff;
                count++;
            }
        }
    }

    if (count > 0) {
        steering /= count;
        if (length(steering) > 0) {
            steering = normalize(steering) * boid.max_speed;
            steering -= entity_motion.velocity;
            if (length(steering) > boid.max_force) {
                steering = normalize(steering) * boid.max_force;
            }
        }
    }
    return steering;
}

vec2 AISystem::calculateAlignment(Entity entity) {
    Motion& entity_motion = registry.motions.get(entity);
    Boid& boid = registry.boids.get(entity);
    vec2 steering(0, 0);
    int count = 0;

    for (Entity other : registry.boids.entities) {
        if (other.id != entity.id) {
            Motion& other_motion = registry.motions.get(other);
            float d = length(other_motion.position - entity_motion.position);

            if (d > 0 && d < boid.search_radius.x) {
                steering += other_motion.velocity;
                count++;
            }
        }
    }

    if (count > 0) {
        steering /= count;
        steering = normalize(steering) * boid.max_speed;
        steering -= entity_motion.velocity;
        if (length(steering) > boid.max_force) {
            steering = normalize(steering) * boid.max_force;
        }
    }
    return steering;
}

vec2 AISystem::calculateCohesion(Entity entity) {
    Motion& entity_motion = registry.motions.get(entity);
    Boid& boid = registry.boids.get(entity);
    vec2 center(0, 0);
    int count = 0;

    for (Entity other : registry.boids.entities) {
        if (other.id != entity.id) {
            Motion& other_motion = registry.motions.get(other);
            float d = length(other_motion.position - entity_motion.position);

            if (d > 0 && d < boid.search_radius.x) {
                center += other_motion.position;
                count++;
            }
        }
    }

    if (count > 0) {
        center /= count;
        vec2 desired = center - entity_motion.position;
        desired = normalize(desired) * boid.max_speed;
        vec2 steering = desired - entity_motion.velocity;
        if (length(steering) > boid.max_force) {
            steering = normalize(steering) * boid.max_force;
        }
        return steering;
    }
    return vec2(0, 0);
}

vec2 AISystem::chasePlayer(Entity entity) {
    if (registry.players.entities.empty()) return vec2(0, 0);

    Motion& entity_motion = registry.motions.get(entity);
    Boid& boid = registry.boids.get(entity);
    Entity player = registry.players.entities[0];
    Motion& player_motion = registry.motions.get(player);

    vec2 to_player = player_motion.position - entity_motion.position;
    float dist = length(to_player);

    if (dist < boid.attack_radius.x) {

        return normalize(-to_player) * boid.max_speed - entity_motion.velocity;
    }
    else {

        return normalize(to_player) * boid.max_speed - entity_motion.velocity;
    }
} 

vec2 AISystem::calculateWander(Entity entity) {
    Motion& motion = registry.motions.get(entity);

    static float angle = 0.0f;
    angle += ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * 0.5f;

    vec2 circle_center = normalize(motion.velocity);
    vec2 displacement = vec2(cos(angle), sin(angle)) * 50.f;

    return normalize(circle_center + displacement) * 40.f;
}

void AISystem::step(float elapsed_ms) {
    float dt = elapsed_ms / 1000.f;

    for (Entity entity : registry.boids.entities) {
        Boid& boid = registry.boids.get(entity);
        Motion& motion = registry.motions.get(entity);

        vec2 separation = calculateSeparation(entity) * boid.separation_weight;
        vec2 alignment = calculateAlignment(entity) * boid.alignment_weight;
        vec2 cohesion = calculateCohesion(entity) * boid.cohesion_weight;

        vec2 flocking = separation + alignment + cohesion;

        float flocking_strength = length(flocking);
        vec2 acceleration = flocking;

        if (flocking_strength < boid.max_force * 0.7f) {
            vec2 chase = chasePlayer(entity) * boid.chase_weight;
            vec2 wander = calculateWander(entity) * 0.8f;

            if (length(chase) < 0.1f) {
                acceleration += wander;
            }
            else {
                acceleration += chase;
            }
        }

        motion.velocity += acceleration * dt;

        float speed = length(motion.velocity);
        if (speed < boid.max_speed * 0.5f) { 
            motion.velocity = normalize(motion.velocity) * (boid.max_speed * 0.5f);
        }
        else if (speed > boid.max_speed) {
            motion.velocity = normalize(motion.velocity) * boid.max_speed;
        }

        motion.target_velocity = motion.velocity;
    }
}