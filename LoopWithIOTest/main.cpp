#include <iostream>
#include <vector>
#include <stack>
#include <cstdint>
#include <memory>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include "Vector3.h"

using Vector = math::Vector3<float>;

//Use version with two loops
#define TWO_LOOPS
//Use direct value instead of pointer. Don't run with SEPARATE_POINTER
//#define DIRECT_VALUE
//separate target pointer from struct. Don't run with DIRECT_VALUE
//#define SEPARATE_POINTER
//Writeback to struct only at the end
#define WRITE_AT_END

struct Body
{
	Body ( ) = default;
	Body ( const Vector& position, const Vector& velocity ) : position { position }, velocity { velocity }
	{

	}

	Vector position;
	Vector velocity;
};

class Plane : public Body
{
public:
	Plane ( ) = default;
	Plane ( const Vector& position, const Vector& velocity ) : Body { position, velocity }
	{

	}

	void Update ( const float time )
	{
		position += velocity * time;
	}
};

class Missile : public Body
{
public:
#if defined DIRECT_VALUE
	Missile ( const Vector& target_position ) : target_position { target_position }
	{

	}
	Missile ( const Vector& position, const Vector& target_position ) : Body { position, Vector { } }, target_position { target_position }
	{

	}
#elif defined SEPARATE_POINTER
	Missile ( ) = default;
	Missile ( const Vector& position ) : Body { position, Vector { } }
	{

	}
#else
	Missile ( const Body*const target ) : target { target }
	{

	}
	Missile ( const Vector& position, const Body*const target ) : Body { position, Vector { } }, target { target }
	{

	}
#endif

#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
	void Update ( const float time, const Body*const target )
#else
	void Update ( const float time )
#endif
	{
#if !defined DIRECT_VALUE
		const auto target_position = target->position;
#endif
		const auto relative_position_to_target = target_position - position;
		const auto direction_to_target = relative_position_to_target.Normalized ( );
#if defined WRITE_AT_END
		const auto velocity = direction_to_target * speed;

		position = position + velocity * time;
		this->velocity = velocity;
#else
		velocity = direction_to_target * speed;
		position += velocity * time;
#endif
	}

#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
	void UpdateVelocity ( const Body*const target )
#else
	void UpdateVelocity ( )
#endif
	{
#if !defined DIRECT_VALUE
		const auto target_position = target->position;
#endif
		const auto relative_position_to_target = target_position - position;
		const auto direction_to_target = relative_position_to_target.Normalized ( );
		velocity = direction_to_target * speed;
	}

	void UpdatePosition ( const float time )
	{
#if defined WRITE_AT_END
		position = this->position + velocity * time;
#else
		position += velocity * time;
#endif
	}

private:
	static constexpr float speed = 12.8f;

#if defined DIRECT_VALUE
	Vector target_position;
#elif !defined SEPARATE_POINTER
	const Body* target;
#endif
};

auto GetTime ( )
{
	return std::chrono::steady_clock::now ( );
}

void main ( )
{
	constexpr float delta_time = 0.03f;
	constexpr size_t num_planes = 1000u;
	constexpr size_t num_missiles = 1000u;
	std::vector<Plane> planes;
	std::vector<Missile> missiles;
#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
	std::vector<const Body*> targets;
#endif

	std::mt19937 rng_machine;
	const std::uniform_real_distribution<float> position_rng ( -1000.f, 1000.f );
	const std::uniform_real_distribution<float> speed_rng ( 0.f, 10.f );
	const std::uniform_int_distribution<size_t> target_rng ( 0, num_planes - 1 );

	planes.reserve ( num_planes );
	for ( size_t i = 0u; i < num_planes; ++i )
	{
		const auto x_pos = position_rng ( rng_machine );
		const auto y_pos = position_rng ( rng_machine );
		const auto z_pos = position_rng ( rng_machine );

		const auto speed = speed_rng ( rng_machine );
		
		const Vector position { x_pos, y_pos, z_pos };
		const Vector velocity = position.Normalized ( ) * speed;
	
		planes.emplace_back ( position, velocity );
	}

	missiles.reserve ( num_missiles );
#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
	targets.reserve ( num_missiles );
#endif
	for ( size_t i = 0u; i < num_missiles; ++i )
	{
		const auto x_pos = position_rng ( rng_machine );
		const auto y_pos = position_rng ( rng_machine );
		const auto z_pos = position_rng ( rng_machine );

		const auto target_id = target_rng ( rng_machine );

		const Vector position { x_pos, y_pos, z_pos };
		const auto target = &planes [ target_id ];

#if defined DIRECT_VALUE
		missiles.emplace_back ( position, target->position );
#elif defined SEPARATE_POINTER
		targets.emplace_back ( target );
		missiles.emplace_back ( position );
#else
		missiles.emplace_back ( position, target );
#endif
	}

	constexpr size_t num_tests = 100000u;

	std::vector<long long> data_points;
	data_points.resize ( num_tests );

	for ( auto data_point_it = data_points.begin ( ), end = data_points.end ( ); data_point_it < end; ++data_point_it )
	{
#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
		auto target_it = targets.begin ( );
#endif
		const auto start_time = GetTime ( );
#if defined TWO_LOOPS
#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
		for ( auto missile_it = missiles.begin ( ), end = missiles.end ( ); missile_it != end; ++missile_it, ++target_it )
		{
			missile_it->UpdateVelocity ( *target_it );
#else
		for ( auto missile_it = missiles.begin ( ), end = missiles.end ( ); missile_it != end; ++missile_it )
		{
			missile_it->UpdateVelocity ( );
#endif
		}
		for ( auto missile_it = missiles.begin ( ), end = missiles.end ( ); missile_it != end; ++missile_it )
		{
			missile_it->UpdatePosition ( delta_time );
		}
#else
#if defined SEPARATE_POINTER && !defined DIRECT_VALUE
		for ( auto missile_it = missiles.begin ( ), end = missiles.end ( ); missile_it != end; ++missile_it )
		{
			missile_it->Update ( delta_time, *target_it );
#else
		for ( auto missile_it = missiles.begin ( ), end = missiles.end ( ); missile_it != end; ++missile_it )
		{
			missile_it->Update ( delta_time );
#endif
		}
#endif

		const auto end_time = GetTime ( );
		const auto duration = end_time - start_time;
		const auto duration_in_nanoseconds = std::chrono::duration_cast< std::chrono::nanoseconds > ( duration );

		*data_point_it = duration_in_nanoseconds.count ( );
	}
	std::sort ( data_points.begin ( ), data_points.end ( ) );
	const auto lowest = data_points.front ( );
	const auto highest = data_points.back ( );
	const auto median = data_points [ ( data_points.size ( ) + 1 ) / 2 ];
	const auto sum = std::accumulate ( data_points.begin ( ), data_points.end ( ), 0ull );
	const auto mean = sum / data_points.size ( );
	const auto standard_deviation = std::sqrt ( static_cast < double > ( std::accumulate ( data_points.begin ( ), data_points.end ( ), 0ull, [ mean ] ( const auto& total, const auto& add )
	{
		const auto difference = add - mean;
		return total + difference * difference;
	} ) ) / data_points.size ( ) );

	std::cout << "lowest: " << lowest << "ns\n";
	std::cout << "highest: " << highest << "ns\n";
	std::cout << "median: " << median << "ns\n";
	std::cout << "mean: " << mean << "ns\n";
	std::cout << "standard deviation: " << standard_deviation << "ns\n";

	system ( "pause" );
}