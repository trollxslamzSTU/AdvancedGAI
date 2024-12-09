#include "sdl_game.h"

using namespace sdl_game;

std::optional<SDL_FPoint> circle_shape::has_rect_intersection(SDL_FRect const & rect) const
{
	SDL_FPoint const closest_point =
	{
		.x = (_origin.x < rect.x) ? rect.x : (_origin.x > rect.x + rect.w) ? rect.x + rect.w : _origin.x,
		.y = (_origin.y < rect.y) ? rect.y : (_origin.y > rect.y + rect.h) ? rect.y + rect.h : _origin.y,
	};

	float const distance = point_distance(_origin, closest_point);

	if (distance <= _radius)
	{
		return closest_point;
	}

	return std::nullopt;
}

std::optional<SDL_FPoint> circle_shape::has_line_intersection(SDL_FPoint const & p1, SDL_FPoint const & p2) const
{
	SDL_FPoint const p1_to_p2 = {p2.x - p1.x, p2.y - p1.y};
	SDL_FPoint const origin_to_p1 = {p1.x - _origin.x, p1.y - _origin.y};
	// Quadratic coefficients.
	float const a = p1_to_p2.x * p1_to_p2.x + p1_to_p2.y * p1_to_p2.y;
	float const b = 2 * (origin_to_p1.x * p1_to_p2.x + origin_to_p1.y * p1_to_p2.y);
	float const c = (origin_to_p1.x * origin_to_p1.x + origin_to_p1.y * origin_to_p1.y) - (_radius * _radius);
	// Quadratic discriminant.
	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0)
	{
		return std::nullopt;
	}

	discriminant = std::sqrt(discriminant);

	float const t1 = (-b - discriminant) / (2 * a);
	float const t2 = (-b + discriminant) / (2 * a);

	// Find the points of intersection (if any)
	SDL_FPoint const intersection1 = {p1.x + t1 * p1_to_p2.x, p1.y + t1 * p1_to_p2.y};
	SDL_FPoint const intersection2 = {p1.x + t2 * p1_to_p2.x, p1.y + t2 * p1_to_p2.y};

	auto const is_point_on_segment = [](SDL_FPoint const & p1, SDL_FPoint const & p2, SDL_FPoint const & point)
	{
		return (std::min(p1.x, p2.x) <= point.x && point.x <= std::max(p1.x, p2.x)) &&
			   (std::min(p1.y, p2.y) <= point.y && point.y <= std::max(p1.y, p2.y));
	};

	bool is_within_segment1 = is_point_on_segment(p1, p2, intersection1);
	bool is_within_segment2 = is_point_on_segment(p1, p2, intersection2);

	if (is_within_segment1)
	{
		return intersection1;
	}

	if (is_within_segment2)
	{
		return intersection2;
	}

	return std::nullopt;
}

bool circle_shape::is_circle_intersecting(SDL_FPoint const & origin, float const & radius) const
{
	float const distance = point_distance(_origin, origin);
	float const sum_of_radii = _radius + radius;

	return distance <= sum_of_radii;
}
