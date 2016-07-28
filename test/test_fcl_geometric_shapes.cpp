/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2016, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */

#include <gtest/gtest.h>

#include "fcl/narrowphase/narrowphase.h"
#include "fcl/collision.h"
#include "test_fcl_utility.h"
#include "fcl/ccd/motion.h"
#include <iostream>
#include <limits>

using namespace fcl;

FCL_REAL extents [6] = {0, 0, 0, 10, 10, 10};

GJKSolver_libccd solver1;
GJKSolver_indep solver2;

#define EXPECT_TRUE_FALSE(p) EXPECT_TRUE(!(p))

GTEST_TEST(FCL_GEOMETRIC_SHAPES, sphere_shape)
{
  const double tol = 1e-12;
  const double radius = 5.0;
  const double pi = constants::pi;

  Sphere s(radius);

  const double volume = 4.0 / 3.0 * pi * radius * radius * radius;
  EXPECT_NEAR(volume, s.computeVolume(), tol);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, gjkcache)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  CollisionRequest request;
  request.enable_cached_gjk_guess = true;
  request.gjk_solver_type = GST_INDEP;

  TranslationMotion motion(Transform3d(Eigen::Translation3d(Vector3d(-20.0, -20.0, -20.0))), Transform3d(Eigen::Translation3d(Vector3d(20.0, 20.0, 20.0))));

  int N = 1000;
  FCL_REAL dt = 1.0 / (N - 1);

  /// test exploiting spatial coherence
  Timer timer1;
  timer1.start();
  std::vector<bool> result1(N);
  for(int i = 0; i < N; ++i)
  {
    motion.integrate(dt * i);
    Transform3d tf;
    motion.getCurrentTransform(tf);

    CollisionResult result;

    collide(&s1, Transform3d::Identity(), &s2, tf, request, result);
    result1[i] = result.isCollision();
    request.cached_gjk_guess = result.cached_gjk_guess; // use cached guess
  }

  timer1.stop();

  /// test without exploiting spatial coherence
  Timer timer2;
  timer2.start();
  std::vector<bool> result2(N);
  request.enable_cached_gjk_guess = false;
  for(int i = 0; i < N; ++i)
  {
    motion.integrate(dt * i);
    Transform3d tf;
    motion.getCurrentTransform(tf);

    CollisionResult result;

    collide(&s1, Transform3d::Identity(), &s2, tf, request, result);
    result2[i] = result.isCollision();
  }

  timer2.stop();

  std::cout << timer1.getElapsedTime() << " " << timer2.getElapsedTime() << std::endl;

  for(std::size_t i = 0; i < result1.size(); ++i)
  {
    EXPECT_TRUE(result1[i] == result2[i]);
  }
}

template <typename S1, typename S2>
void printComparisonError(const std::string& comparison_type,
                          const S1& s1, const Transform3d& tf1,
                          const S2& s2, const Transform3d& tf2,
                          GJKSolverType solver_type,
                          const Vector3d& expected_contact_or_normal,
                          const Vector3d& actual_contact_or_normal,
                          bool check_opposite_normal,
                          FCL_REAL tol)
{
  std::cout << "Disagreement between " << comparison_type
            << " and expected_" << comparison_type << " for "
            << getNodeTypeName(s1.getNodeType()) << " and "
            << getNodeTypeName(s2.getNodeType()) << " with '"
            << getGJKSolverName(solver_type) << "' solver." << std::endl
            << "tf1.linear: " << tf1.linear() << std::endl
            << "tf1.translation: " << tf1.translation() << std::endl
            << "tf2.linear: " << tf2.linear() << std::endl
            << "tf2.translation: " << tf2.translation() << std::endl
            << "expected_" << comparison_type << ": " << expected_contact_or_normal
            << "actual_" << comparison_type << "  : " << actual_contact_or_normal << std::endl;

  if (check_opposite_normal)
    std::cout << " or " << -expected_contact_or_normal;

  std::cout << std::endl
            << "difference: " << (actual_contact_or_normal - expected_contact_or_normal).norm() << std::endl
            << "tolerance: " << tol << std::endl;
}

template <typename S1, typename S2>
void printComparisonError(const std::string& comparison_type,
                          const S1& s1, const Transform3d& tf1,
                          const S2& s2, const Transform3d& tf2,
                          GJKSolverType solver_type,
                          FCL_REAL expected_depth,
                          FCL_REAL actual_depth,
                          FCL_REAL tol)
{
  std::cout << "Disagreement between " << comparison_type
            << " and expected_" << comparison_type << " for "
            << getNodeTypeName(s1.getNodeType()) << " and "
            << getNodeTypeName(s2.getNodeType()) << " with '"
            << getGJKSolverName(solver_type) << "' solver." << std::endl
            << "tf1.linear: " << tf1.linear() << std::endl
            << "tf1.translation: " << tf1.translation() << std::endl
            << "tf2.linear: " << tf2.linear() << std::endl
            << "tf2.translation: " << tf2.translation() << std::endl
            << "expected_depth: " << expected_depth << std::endl
            << "actual_depth  : " << actual_depth << std::endl
            << "difference: " << std::abs(actual_depth - expected_depth) << std::endl
            << "tolerance: " << tol << std::endl;
}

template <typename S1, typename S2>
bool checkContactPoints(const S1& s1, const Transform3d& tf1,
                        const S2& s2, const Transform3d& tf2,
                        GJKSolverType solver_type,
                        const ContactPoint& expected, const ContactPoint& actual,
                        bool check_position = false,
                        bool check_depth = false,
                        bool check_normal = false,
                        bool check_opposite_normal = false,
                        FCL_REAL tol = 1e-9)
{
  if (check_position)
  {
    bool contact_equal = actual.pos.isApprox(expected.pos, tol);
    if (!contact_equal)
      return false;
  }

  if (check_depth)
  {
    bool depth_equal = std::abs(actual.penetration_depth - expected.penetration_depth) < tol;
    if (!depth_equal)
      return false;
  }

  if (check_normal)
  {
    bool normal_equal = actual.normal.isApprox(expected.normal, tol);

    if (!normal_equal && check_opposite_normal)
      normal_equal = actual.normal.isApprox(-expected.normal, tol);

    if (!normal_equal)
      return false;
  }

  return true;
}

template <typename S1, typename S2>
bool inspectContactPoints(const S1& s1, const Transform3d& tf1,
                          const S2& s2, const Transform3d& tf2,
                          GJKSolverType solver_type,
                          const std::vector<ContactPoint>& expected_contacts,
                          const std::vector<ContactPoint>& actual_contacts,
                          bool check_position = false,
                          bool check_depth = false,
                          bool check_normal = false,
                          bool check_opposite_normal = false,
                          FCL_REAL tol = 1e-9)
{
  // Check number of contact points
  bool sameNumContacts = (actual_contacts.size() == expected_contacts.size());
  EXPECT_TRUE(sameNumContacts);
  if (!sameNumContacts)
  {
    std::cout << "\n"
              << "===== [ geometric shape collision test failure report ] ======\n"
              << "\n"
              << "Solver type: " << getGJKSolverName(solver_type) << "\n"
              << "\n"
              << "[ Shape 1 ]\n"
              << "Shape type     : " << getNodeTypeName(s1.getNodeType()) << "\n"
              << "tf1.linear     : " << tf1.linear() << "\n"
              << "tf1.translation: " << tf1.translation() << "\n"
              << "\n"
              << "[ Shape 2 ]\n"
              << "Shape type     : " << getNodeTypeName(s2.getNodeType()) << "\n"
              << "tf2.linear     : " << tf2.linear() << "\n"
              << "tf2.translation: " << tf2.translation() << "\n"
              << "\n"
              << "The numbers of expected contacts '"
              << expected_contacts.size()
              << "' and the number of actual contacts '"
              << actual_contacts.size()
              << "' are not equal.\n"
              << "\n";
    return false;
  }

  // Check if actual contacts and expected contacts are matched
  const size_t numContacts = actual_contacts.size();

  std::vector<int> index_to_actual_contacts(numContacts, -1);
  std::vector<int> index_to_expected_contacts(numContacts, -1);

  bool foundAll = true;
  for (size_t i = 0; i < numContacts; ++i)
  {
    const ContactPoint& expected = expected_contacts[i];

    // Check if expected contact is in the list of actual contacts
    for (size_t j = 0; j < numContacts; ++j)
    {
      if (index_to_expected_contacts[j] != -1)
        continue;

      const ContactPoint& actual = actual_contacts[j];

      bool found = checkContactPoints(
            s1, tf1, s2, tf2, solver_type,
            expected, actual,
            check_position,
            check_depth,
            check_normal, check_opposite_normal,
            tol);

      if (found)
      {
        index_to_actual_contacts[i] = j;
        index_to_expected_contacts[j] = i;
        break;
      }
    }

    if (index_to_actual_contacts[i] == -1)
      foundAll = false;
  }

  if (!foundAll)
  {
    std::cout << "\n"
              << "===== [ geometric shape collision test failure report ] ======\n"
              << "\n"
              << "Solver type: " << getGJKSolverName(solver_type) << "\n"
              << "\n"
              << "[ Shape 1 ]\n"
              << "Shape type     : " << getNodeTypeName(s1.getNodeType()) << "\n"
              << "tf1.linear     : " << tf1.linear() << "\n"
              << "tf1.translation: " << tf1.translation() << "\n"
              << "\n"
              << "[ Shape 2 ]\n"
              << "Shape type     : " << getNodeTypeName(s2.getNodeType()) << "\n"
              << "tf2.linear     : " << tf2.linear() << "\n"
              << "tf2.translation: " << tf2.translation() << "\n"
              << "\n"
              << "[ Expected Contacts: " << numContacts << " ]\n";
    for (size_t i = 0; i < numContacts; ++i)
    {
      const ContactPoint& expected = expected_contacts[i];

      std::cout << "(" << i << ") pos: " << expected.pos << ", "
                << "normal: " << expected.normal << ", "
                << "depth: " << expected.penetration_depth
                << " ---- ";
      if (index_to_actual_contacts[i] != -1)
        std::cout << "found, actual (" << index_to_actual_contacts[i] << ")\n";
      else
        std::cout << "not found!\n";
    }
    std::cout << "\n"
              << "[ Actual Contacts: " << numContacts << " ]\n";
    for (size_t i = 0; i < numContacts; ++i)
    {
      const ContactPoint& actual = actual_contacts[i];

      std::cout << "(" << i << ") pos: " << actual.pos << ", "
                << "normal: " << actual.normal << ", "
                << "depth: " << actual.penetration_depth
                << " ---- ";
      if (index_to_expected_contacts[i] != -1)
        std::cout << "found, expected (" << index_to_expected_contacts[i] << ")\n";
      else
        std::cout << "not found!\n";
    }

    std::cout << "\n";
  }

  return foundAll;
}

void getContactPointsFromResult(std::vector<ContactPoint>& contacts, const CollisionResult& result)
{
  const size_t numContacts = result.numContacts();
  contacts.resize(numContacts);

  for (size_t i = 0; i < numContacts; ++i)
  {
    const Contact& cnt = result.getContact(i);

    contacts[i].pos = cnt.pos;
    contacts[i].normal = cnt.normal;
    contacts[i].penetration_depth = cnt.penetration_depth;
  }
}

template <typename S1, typename S2>
void testShapeIntersection(
    const S1& s1, const Transform3d& tf1,
    const S2& s2, const Transform3d& tf2,
    GJKSolverType solver_type,
    bool expected_res,
    const std::vector<ContactPoint>& expected_contacts = std::vector<ContactPoint>(),
    bool check_position = true,
    bool check_depth = true,
    bool check_normal = true,
    bool check_opposite_normal = false,
    FCL_REAL tol = 1e-9)
{
  CollisionRequest request;
  request.gjk_solver_type = solver_type;
  request.num_max_contacts = std::numeric_limits<size_t>::max();
  CollisionResult result;

  std::vector<ContactPoint> actual_contacts;

  bool res;

  // Part A: Check collisions using shapeIntersect()

  // Check only whether they are colliding or not.
  if (solver_type == GST_LIBCCD)
  {
    res = solver1.shapeIntersect(s1, tf1, s2, tf2, NULL);
  }
  else if (solver_type == GST_INDEP)
  {
    res = solver2.shapeIntersect(s1, tf1, s2, tf2, NULL);
  }
  else
  {
    std::cerr << "Invalid GJK solver. Test aborted." << std::endl;
    return;
  }
  EXPECT_EQ(res, expected_res);

  // Check contact information as well
  if (solver_type == GST_LIBCCD)
  {
    res = solver1.shapeIntersect(s1, tf1, s2, tf2, &actual_contacts);
  }
  else if (solver_type == GST_INDEP)
  {
    res = solver2.shapeIntersect(s1, tf1, s2, tf2, &actual_contacts);
  }
  else
  {
    std::cerr << "Invalid GJK solver. Test aborted." << std::endl;
    return;
  }
  EXPECT_EQ(res, expected_res);
  if (expected_res)
  {
    EXPECT_TRUE(inspectContactPoints(s1, tf1, s2, tf2, solver_type,
                                     expected_contacts, actual_contacts,
                                     check_position,
                                     check_depth,
                                     check_normal, check_opposite_normal,
                                     tol));
  }

  // Part B: Check collisions using collide()

  // Check only whether they are colliding or not.
  request.enable_contact = false;
  result.clear();
  res = (collide(&s1, tf1, &s2, tf2, request, result) > 0);
  EXPECT_EQ(res, expected_res);

  // Check contact information as well
  request.enable_contact = true;
  result.clear();
  res = (collide(&s1, tf1, &s2, tf2, request, result) > 0);
  EXPECT_EQ(res, expected_res);
  if (expected_res)
  {
    getContactPointsFromResult(actual_contacts, result);
    EXPECT_TRUE(inspectContactPoints(s1, tf1, s2, tf2, solver_type,
                                     expected_contacts, actual_contacts,
                                     check_position,
                                     check_depth,
                                     check_normal, check_opposite_normal,
                                     tol));
  }
}

// Shape intersection test coverage (libccd)
//
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// |            | box | sphere | ellipsoid | capsule | cone | cylinder | plane | half-space | triangle |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | box        |  O  |   O    |           |         |      |          |   O   |      O     |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | sphere     |/////|   O    |           |         |      |          |   O   |      O     |    O     |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | ellipsoid  |/////|////////|     O     |         |      |          |   O   |      O     |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | capsule    |/////|////////|///////////|         |      |          |   O   |      O     |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cone       |/////|////////|///////////|/////////|  O   |    O     |   O   |      O     |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cylinder   |/////|////////|///////////|/////////|//////|    O     |   O   |      O     |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | plane      |/////|////////|///////////|/////////|//////|//////////|       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | half-space |/////|////////|///////////|/////////|//////|//////////|///////|            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | triangle   |/////|////////|///////////|/////////|//////|//////////|///////|////////////|          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  contacts[0].pos << 20, 0, 0;
  contacts[0].penetration_depth = 0.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  contacts[0].pos << 20.0 - 0.1 * 20.0/(20.0 + 10.0), 0, 0;
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[0].pos = transform * Vector3d(20.0 - 0.1 * 20.0/(20.0 + 10.0), 0, 0);
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  contacts[0].pos.setZero();
  contacts[0].penetration_depth = 20.0 + 10.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  contacts[0].pos = transform * Vector3d::Zero();
  contacts[0].penetration_depth = 20.0 + 10.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << -1, 0, 0;
  contacts[0].pos << -20.0 + 0.1 * 20.0/(20.0 + 10.0), 0, 0;
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  contacts[0].pos = transform * Vector3d(-20.0 + 0.1 * 20.0/(20.0 + 10.0), 0, 0);
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30.0, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << -1, 0, 0;
  contacts[0].pos << -20, 0, 0;
  contacts[0].penetration_depth = 0.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

bool compareContactPoints1(const Vector3d& c1,const Vector3d& c2)
{
  return c1[2] < c2[2];
} // Ascending order

bool compareContactPoints2(const ContactPoint& cp1,const ContactPoint& cp2)
{
  return cp1.pos[2] < cp2.pos[2];
} // Ascending order

void testBoxBoxContactPoints(const Matrix3d& R)
{
  Box s1(100, 100, 100);
  Box s2(10, 20, 30);

  // Vertices of s2
  std::vector<Vector3d> vertices(8);
  vertices[0] <<  1,  1,  1;
  vertices[1] <<  1,  1, -1;
  vertices[2] <<  1, -1,  1;
  vertices[3] <<  1, -1, -1;
  vertices[4] << -1,  1,  1;
  vertices[5] << -1,  1, -1;
  vertices[6] << -1, -1,  1;
  vertices[7] << -1, -1, -1;

  for (int i = 0; i < 8; ++i)
  {
    vertices[i][0] *= 0.5 * s2.side[0];
    vertices[i][1] *= 0.5 * s2.side[1];
    vertices[i][2] *= 0.5 * s2.side[2];
  }

  Transform3d tf1 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -50)));
  Transform3d tf2 = Transform3d(R);

  std::vector<ContactPoint> contacts;

  // Make sure the two boxes are colliding
  bool res = solver1.shapeIntersect(s1, tf1, s2, tf2, &contacts);
  EXPECT_TRUE(res);

  // Compute global vertices
  for (int i = 0; i < 8; ++i)
    vertices[i] = tf2 * vertices[i];

  // Sort the vertices so that the lowest vertex along z-axis comes first
  std::sort(vertices.begin(), vertices.end(), compareContactPoints1);
  std::sort(contacts.begin(), contacts.end(), compareContactPoints2);

  // The lowest n vertex along z-axis should be the contact point
  size_t numContacts = contacts.size();
  numContacts = std::min(static_cast<size_t>(1), numContacts);
  // TODO: BoxBox algorithm seems not able to find all the colliding vertices.
  // We just check the deepest one as workaround.
  for (size_t i = 0; i < numContacts; ++i)
  {
    EXPECT_TRUE(vertices[i].isApprox(contacts[i].pos));
    EXPECT_TRUE(Vector3d(0, 0, 1).isApprox(contacts[i].normal));
  }
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  Quaternion3d q(Eigen::AngleAxisd((FCL_REAL)3.140 / 6, Vector3d(0, 0, 1)));

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position. The current result is (1, 0, 0).
  contacts.resize(4);
  contacts[0].normal << 1, 0, 0;
  contacts[1].normal << 1, 0, 0;
  contacts[2].normal << 1, 0, 0;
  contacts[3].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position. The current result is (1, 0, 0).
  contacts.resize(4);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[1].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[2].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[3].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(15, 0, 0)));
  contacts.resize(4);
  contacts[0].normal = Vector3d(1, 0, 0);
  contacts[1].normal = Vector3d(1, 0, 0);
  contacts[2].normal = Vector3d(1, 0, 0);
  contacts[3].normal = Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(15.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(q);
  contacts.resize(4);
  contacts[0].normal = Vector3d(1, 0, 0);
  contacts[1].normal = Vector3d(1, 0, 0);
  contacts[2].normal = Vector3d(1, 0, 0);
  contacts[3].normal = Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(q);
  contacts.resize(4);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[1].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[2].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[3].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  FCL_UINT32 numTests = 1e+2;
  for (FCL_UINT32 i = 0; i < numTests; ++i)
  {
    Transform3d tf;
    generateRandomTransform(extents, tf);
    testBoxBoxContactPoints(tf.linear());
  }
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_spherebox)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position. The current result is (-1, 0, 0).
  contacts.resize(1);
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(22.5, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(22.501, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(22.4, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(22.4, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 1e-4);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_spherecapsule)
{
  Sphere s1(20);
  Capsule s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(24.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(24.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(25, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  //tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(25, 0, 0)));
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(25 - 1e-6, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(25.1, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(25.1, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 1e-5);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 1e-5);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.001, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.001, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 1e-5);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_cylindercone)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 0.061);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 0.46);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, true, false, 1e-5);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.01)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.01)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_ellipsoidellipsoid)
{
  Ellipsoid s1(20, 40, 50);
  Ellipsoid s2(10, 10, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);
  Transform3d identity;

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_spheretriangle)
{
  Sphere s(10);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  Vector3d normal;
  bool res;

  res = solver1.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);


  t[0] << 30, 0, 0;
  t[1] << 9.9, -20, 0;
  t[2] << 9.9, 20, 0;
  res = solver1.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver1.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver1.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacetriangle)
{
  Halfspace hs(Vector3d(1, 0, 0), 0);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  // Vector3d point;
  // FCL_REAL depth;
  Vector3d normal;
  bool res;

  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);


  t[0] << 20, 0, 0;
  t[1] << 0, -20, 0;
  t[2] << 0, 20, 0;
  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planetriangle)
{
  Plane hs(Vector3d(1, 0, 0), 0);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  // Vector3d point;
  // FCL_REAL depth;
  Vector3d normal;
  bool res;

  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);


  t[0] << 20, 0, 0;
  t[1] << -0.1, -20, 0;
  t[2] << -0.1, 20, 0;
  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacesphere)
{
  Sphere s(10);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -5, 0, 0;
  contacts[0].penetration_depth = 10;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-5, 0, 0);
  contacts[0].penetration_depth = 10;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 15;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 15;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -7.5, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-7.5, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.05, 0, 0;
  contacts[0].penetration_depth = 20.1;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.05, 0, 0);
  contacts[0].penetration_depth = 20.1;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planesphere)
{
  Sphere s(10);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos.setZero();
  contacts[0].penetration_depth = 10;
  contacts[0].normal << 1, 0, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 10;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 5, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(5, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -5, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-5, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacebox)
{
  Box s(5, 10, 20);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -0.625, 0, 0;
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-0.625, 0, 0);
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.875, 0, 0;
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.875, 0, 0);
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.51, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.005, 0, 0;
  contacts[0].penetration_depth = 5.01;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.51, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.005, 0, 0);
  contacts[0].penetration_depth = 5.01;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d(transform.linear());
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, false, false, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planebox)
{
  Box s(5, 10, 20);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 1, 0, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 1.25, 0, 0;
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(1.25, 0, 0);
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, 0;
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, 0);
  contacts[0].penetration_depth = 1.25;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.51, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d(transform.linear());
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, false, false, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspaceellipsoid)
{
  Ellipsoid s(5, 10, 20);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 5.0;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 5.0;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.875, 0, 0;
  contacts[0].penetration_depth = 6.25;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.875, 0, 0);
  contacts[0].penetration_depth = 6.25;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -3.125, 0, 0;
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-3.125, 0, 0);
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.01, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.005, 0, 0;
  contacts[0].penetration_depth = 10.01;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.01, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.005, 0, 0);
  contacts[0].penetration_depth = 10.01;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, -5.0, 0;
  contacts[0].penetration_depth = 10.0;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -5.0, 0);
  contacts[0].penetration_depth = 10.0;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 1.25, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -4.375, 0;
  contacts[0].penetration_depth = 11.25;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 1.25, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -4.375, 0);
  contacts[0].penetration_depth = 11.25;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -1.25, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -5.625, 0;
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -1.25, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -5.625, 0);
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 10.01, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 0.005, 0;
  contacts[0].penetration_depth = 20.01;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 10.01, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0.005, 0);
  contacts[0].penetration_depth = 20.01;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, -10.0;
  contacts[0].penetration_depth = 20.0;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -10.0);
  contacts[0].penetration_depth = 20.0;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 1.25)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -9.375;
  contacts[0].penetration_depth = 21.25;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 1.25)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -9.375);
  contacts[0].penetration_depth = 21.25;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -1.25)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -10.625;
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -1.25)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -10.625);
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 20.01)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0.005;
  contacts[0].penetration_depth = 40.01;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 20.01)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0.005);
  contacts[0].penetration_depth = 40.01;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planeellipsoid)
{
  Ellipsoid s(5, 10, 20);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5.0;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5.0;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 1.25, 0, 0;
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(1.25, 0, 0);
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, 0;
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-1.25, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, 0);
  contacts[0].penetration_depth = 3.75;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.01, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0.0, 0;
  contacts[0].penetration_depth = 10.0;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 10.0;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 1.25, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 1.25, 0;
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal << 0, 1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 1.25, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 1.25, 0);
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -1.25, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -1.25, 0;
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -1.25, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -1.25, 0);
  contacts[0].penetration_depth = 8.75;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -10.01, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 20.0;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 20.0;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 1.25)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 1.25;
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 1.25)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 1.25);
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -1.25)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -1.25;
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -1.25)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -1.25);
  contacts[0].penetration_depth = 18.75;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -20.01)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacecapsule)
{
  Capsule s(5, 10);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, 0;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, 0);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -3.75, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-3.75, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.05, 0, 0;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.05, 0, 0);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -1.25, 0;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -1.25, 0);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -3.75, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -3.75, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 0.05, 0;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0.05, 0);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, -5;
  contacts[0].penetration_depth = 10;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -5);
  contacts[0].penetration_depth = 10;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -3.75;
  contacts[0].penetration_depth = 12.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -3.75);
  contacts[0].penetration_depth = 12.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -6.25;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -6.25);
  contacts[0].penetration_depth  = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0.05;
  contacts[0].penetration_depth = 20.1;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0.05);
  contacts[0].penetration_depth = 20.1;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planecapsule)
{
  Capsule s(5, 10);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 1, 0, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 2.5, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(2.5, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 1, 0;  // (0, 1, 0) or (0, -1, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);  // (0, 1, 0) or (0, -1, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 2.5, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 2.5, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 10;
  contacts[0].normal << 0, 0, 1;  // (0, 0, 1) or (0, 0, -1)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 10;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);  // (0, 0, 1) or (0, 0, -1)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 2.5;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 2.5);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -2.5;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -2.5);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacecylinder)
{
  Cylinder s(5, 10);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, 0;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, 0);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -3.75, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-3.75, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.05, 0, 0;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.05, 0, 0);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -1.25, 0;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -1.25, 0);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -3.75, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -3.75, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 0.05, 0;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0.05, 0);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, -2.5;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -2.5);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -1.25;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -1.25);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -3.75;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -3.75);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 5.1)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0.05;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 5.1)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0.05);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -5.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -5.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planecylinder)
{
  Cylinder s(5, 10);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 1, 0, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 2.5, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(2.5, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 1, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 2.5, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 2.5, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, 0;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, 0);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 0, 1;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}


GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_halfspacecone)
{
  Cone s(5, 10);
  Halfspace hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, -5;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, -5);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -1.25, 0, -5;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-1.25, 0, -5);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -3.75, 0, -5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-3.75, 0, -5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 0.05, 0, -5;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0.05, 0, -5);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, -5;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, -5);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -1.25, -5;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -1.25, -5);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -3.75, -5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -3.75, -5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 0.05, -5;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0.05, -5);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Halfspace(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, -2.5;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -2.5);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -1.25;
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -1.25);
  contacts[0].penetration_depth = 7.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -3.75;
  contacts[0].penetration_depth= 2.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -3.75);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 5.1)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0.05;
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 5.1)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0.05);
  contacts[0].penetration_depth = 10.1;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -5.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -5.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersection_planecone)
{
  Cone s(5, 10);
  Plane hs(Vector3d(1, 0, 0), 0);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 1, 0, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << 2.5, 0, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(2.5, 0, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos << -2.5, 0, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << -1, 0, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-2.5, 0, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(-2.5, 0, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-5.1, 0, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 1, 0), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 1, 0;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, 2.5, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 2.5, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos << 0, -2.5, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, -1, 0;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -2.5, 0)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, -2.5, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, -1, 0);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, -5.1, 0)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);




  hs = Plane(Vector3d(0, 0, 1), 0);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].pos << 0, 0, 0;
  contacts[0].penetration_depth = 5;
  contacts[0].normal << 0, 0, 1;  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 0);
  contacts[0].penetration_depth = 5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);  // (1, 0, 0) or (-1, 0, 0)
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts, true, true, true, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, 2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, 2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos << 0, 0, -2.5;
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal << 0, 0, -1;
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -2.5)));
  contacts.resize(1);
  contacts[0].pos = transform * Vector3d(0, 0, -2.5);
  contacts[0].penetration_depth = 2.5;
  contacts[0].normal = transform.linear() * Vector3d(0, 0, -1);
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, -10.1)));
  testShapeIntersection(s, tf1, hs, tf2, GST_LIBCCD, false);
}

// Shape distance test coverage (libccd)
//
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// |            | box | sphere | ellipsoid | capsule | cone | cylinder | plane | half-space | triangle |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | box        |  O  |   O    |           |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | sphere     |/////|   O    |           |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | ellipsoid  |/////|////////|     O     |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | capsule    |/////|////////|///////////|         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cone       |/////|////////|///////////|/////////|  O   |    O     |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cylinder   |/////|////////|///////////|/////////|//////|    O     |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | plane      |/////|////////|///////////|/////////|//////|//////////|       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | half-space |/////|////////|///////////|/////////|//////|//////////|///////|            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | triangle   |/////|////////|///////////|/////////|//////|//////////|///////|////////////|          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3d transform = Transform3d::Identity();
  //generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;
  Vector3d closest_p1, closest_p2;
  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(0, 40, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);


  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  // this is one problem: the precise is low sometimes
  EXPECT_TRUE(fabs(dist - 10) < 0.1);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.06);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.1);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.1);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);
  Vector3d closest_p1, closest_p2;

  Transform3d transform = Transform3d::Identity();
  //generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(20.1, 0, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(0, 20.2, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10.2) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 10.1, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 0.1 * 1.414) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(20.1, 0, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(0, 20.1, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s2, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 10.1, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 0.1 * 1.414) < 0.001);
  EXPECT_TRUE(res);


  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(15.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(20, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 5) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(20, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 5) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_boxsphere)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(22.6, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(22.6, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.05);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 17.5) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 17.5) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(0, 0, 40))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 1);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 40))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 1);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_conecylinder)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.01);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.02);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.01);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.1);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistance_ellipsoidellipsoid)
{
  Ellipsoid s1(20, 40, 50);
  Ellipsoid s2(10, 10, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;
  Vector3d closest_p1, closest_p2;

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist, &closest_p1, &closest_p2);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);


  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver1.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);
}

// Shape intersection test coverage (built-in GJK)
//
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// |            | box | sphere | ellipsoid | capsule | cone | cylinder | plane | half-space | triangle |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | box        |  O  |   O    |           |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | sphere     |/////|   O    |           |         |      |          |       |            |     O    |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | ellipsoid  |/////|////////|     O     |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | capsule    |/////|////////|///////////|         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cone       |/////|////////|///////////|/////////|  O   |    O     |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cylinder   |/////|////////|///////////|/////////|//////|    O     |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | plane      |/////|////////|///////////|/////////|//////|//////////|       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | half-space |/////|////////|///////////|/////////|//////|//////////|///////|            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | triangle   |/////|////////|///////////|/////////|//////|//////////|///////|////////////|          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  contacts[0].pos << 20, 0, 0;
  contacts[0].penetration_depth = 0.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  contacts[0].pos << 20.0 - 0.1 * 20.0/(20.0 + 10.0), 0, 0;
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[0].pos = transform * Vector3d(20.0 - 0.1 * 20.0/(20.0 + 10.0), 0, 0);
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  contacts[0].normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  contacts[0].pos.setZero();
  contacts[0].penetration_depth = 20.0 + 10.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  contacts[0].normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  contacts[0].pos = transform * Vector3d::Zero();
  contacts[0].penetration_depth = 20.0 + 10.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << -1, 0, 0;
  contacts[0].pos << -20.0 + 0.1 * 20.0/(20.0 + 10.0), 0, 0;
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-29.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(-1, 0, 0);
  contacts[0].pos = transform * Vector3d(-20.0 + 0.1 * 20.0/(20.0 + 10.0), 0, 0);
  contacts[0].penetration_depth = 0.1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30.0, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << -1, 0, 0;
  contacts[0].pos << -20, 0, 0;
  contacts[0].penetration_depth = 0.0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  Quaternion3d q(Eigen::AngleAxisd((FCL_REAL)3.140 / 6, Vector3d(0, 0, 1)));

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position. The current result is (1, 0, 0).
  contacts.resize(4);
  contacts[0].normal << 1, 0, 0;
  contacts[1].normal << 1, 0, 0;
  contacts[2].normal << 1, 0, 0;
  contacts[3].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position. The current result is (1, 0, 0).
  contacts.resize(4);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[1].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[2].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[3].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(15, 0, 0)));
  contacts.resize(4);
  contacts[0].normal = Vector3d(1, 0, 0);
  contacts[1].normal = Vector3d(1, 0, 0);
  contacts[2].normal = Vector3d(1, 0, 0);
  contacts[3].normal = Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(15.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(q);
  contacts.resize(4);
  contacts[0].normal = Vector3d(1, 0, 0);
  contacts[1].normal = Vector3d(1, 0, 0);
  contacts[2].normal = Vector3d(1, 0, 0);
  contacts[3].normal = Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(q);
  contacts.resize(4);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[1].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[2].normal = transform.linear() * Vector3d(1, 0, 0);
  contacts[3].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_spherebox)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(22.5, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true, false, 1e-7);  // built-in GJK solver requires larger tolerance than libccd

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(22.51, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(22.4, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true, false, 1e-2);  // built-in GJK solver requires larger tolerance than libccd

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(22.4, 0, 0)));
  contacts.resize(1);
  // contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);
  // built-in GJK solver returns incorrect normal.
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_spherecapsule)
{
  Sphere s1(20);
  Capsule s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(24.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(24.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(25, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(25.1, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true, false, 3e-1);  // built-in GJK solver requires larger tolerance than libccd

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  // contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);
  // built-in GJK solver returns incorrect normal.

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  contacts[0].normal << 1, 0, 0;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true, false, 5.7e-1);  // built-in GJK solver requires larger tolerance than libccd

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  // contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);
  // built-in GJK solver returns incorrect normal.

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  // contacts[0].normal = transform.linear() * Vector3d(0, 0, 1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);
  // built-in GJK solver returns incorrect normal.
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_cylindercone)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  std::vector<ContactPoint> contacts;
  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  // TODO: Need convention for normal when the centers of two objects are at same position.
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(9.9, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(10, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(10, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 9.9)));
  contacts.resize(1);
  // contacts[0].normal = transform.linear() * Vector3d(1, 0, 0);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);
  // built-in GJK solver returns incorrect normal.

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10)));
  contacts.resize(1);
  contacts[0].normal << 0, 0, 1;
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, true);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 10.1)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_ellipsoidellipsoid)
{
  Ellipsoid s1(20, 40, 50);
  Ellipsoid s2(10, 10, 10);

  Transform3d tf1;
  Transform3d tf2;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);
  Transform3d identity;

  std::vector<ContactPoint> contacts;

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(30, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d::Identity();
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform;
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-29.99, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = Transform3d::Identity();
  tf2 = Transform3d(Eigen::Translation3d(Vector3d(-30, 0, 0)));
  contacts.resize(1);
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, true, contacts, false, false, false);

  tf1 = transform;
  tf2 = transform * Transform3d(Eigen::Translation3d(Vector3d(-30.01, 0, 0)));
  testShapeIntersection(s1, tf1, s2, tf2, GST_INDEP, false);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_spheretriangle)
{
  Sphere s(10);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  // Vector3d point;
  // FCL_REAL depth;
  Vector3d normal;
  bool res;

  res = solver2.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver2.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  t[0] << 30, 0, 0;
  t[1] << 9.9, -20, 0;
  t[2] << 9.9, 20, 0;
  res = solver2.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver2.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver2.shapeTriangleIntersect(s, Transform3d::Identity(), t[0], t[1], t[2], NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver2.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_halfspacetriangle)
{
  Halfspace hs(Vector3d(1, 0, 0), 0);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  // Vector3d point;
  // FCL_REAL depth;
  Vector3d normal;
  bool res;

  res = solver2.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver2.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);


  t[0] << 20, 0, 0;
  t[1] << -0.1, -20, 0;
  t[2] << -0.1, 20, 0;
  res = solver2.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver2.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver2.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver2.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeIntersectionGJK_planetriangle)
{
  Plane hs(Vector3d(1, 0, 0), 0);
  Vector3d t[3];
  t[0] << 20, 0, 0;
  t[1] << -20, 0, 0;
  t[2] << 0, 20, 0;

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  // Vector3d point;
  // FCL_REAL depth;
  Vector3d normal;
  bool res;

  res = solver1.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver1.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);


  t[0] << 20, 0, 0;
  t[1] << -0.1, -20, 0;
  t[2] << -0.1, 20, 0;
  res = solver2.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res =  solver2.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  EXPECT_TRUE(res);

  res = solver2.shapeTriangleIntersect(hs, Transform3d::Identity(), t[0], t[1], t[2], Transform3d::Identity(), NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(Vector3d(1, 0, 0), 1e-9));

  res =  solver2.shapeTriangleIntersect(hs, transform, t[0], t[1], t[2], transform, NULL, NULL, &normal);
  EXPECT_TRUE(res);
  EXPECT_TRUE(normal.isApprox(transform.linear() * Vector3d(1, 0, 0), 1e-9));
}

// Shape distance test coverage (built-in GJK)
//
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// |            | box | sphere | ellipsoid | capsule | cone | cylinder | plane | half-space | triangle |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | box        |  O  |   O    |           |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | sphere     |/////|   O    |           |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | ellipsoid  |/////|////////|     O     |         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | capsule    |/////|////////|///////////|         |      |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cone       |/////|////////|///////////|/////////|  O   |          |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | cylinder   |/////|////////|///////////|/////////|//////|    O     |       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | plane      |/////|////////|///////////|/////////|//////|//////////|       |            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | half-space |/////|////////|///////////|/////////|//////|//////////|///////|            |          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+
// | triangle   |/////|////////|///////////|/////////|//////|//////////|///////|////////////|          |
// +------------+-----+--------+-----------+---------+------+----------+-------+------------+----------+

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);


  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(15.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(15.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(20, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 5) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(20, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 5) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_boxsphere)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(22.6, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.01);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(22.6, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.01);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 17.5) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 17.5) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(10.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(0, 0, 40))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(0, 0, 40))), &dist);
  EXPECT_TRUE(fabs(dist - 30) < 0.001);
  EXPECT_TRUE(res);
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, shapeDistanceGJK_ellipsoidellipsoid)
{
  Ellipsoid s1(20, 40, 50);
  Ellipsoid s2(10, 10, 10);

  Transform3d transform = Transform3d::Identity();
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d::Identity(), s2, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, Transform3d::Identity(), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(40, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 10) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(30.1, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(fabs(dist - 0.1) < 0.001);
  EXPECT_TRUE(res);

  res = solver2.shapeDistance(s1, transform * Transform3d(Eigen::Translation3d(Vector3d(29.9, 0, 0))), s2, transform, &dist);
  EXPECT_TRUE(dist < 0);
  EXPECT_TRUE_FALSE(res);
}

template<typename S1, typename S2>
void testReversibleShapeIntersection(const S1& s1, const S2& s2, FCL_REAL distance)
{
  Transform3d tf1(Eigen::Translation3d(Vector3d(-0.5 * distance, 0.0, 0.0)));
  Transform3d tf2(Eigen::Translation3d(Vector3d(+0.5 * distance, 0.0, 0.0)));

  std::vector<ContactPoint> contactsA;
  std::vector<ContactPoint> contactsB;

  bool resA;
  bool resB;

  const double tol = 1e-6;

  resA = solver1.shapeIntersect(s1, tf1, s2, tf2, &contactsA);
  resB = solver1.shapeIntersect(s2, tf2, s1, tf1, &contactsB);

  // normal should be opposite
  for (size_t i = 0; i < contactsB.size(); ++i)
    contactsB[i].normal = -contactsB[i].normal;

  EXPECT_TRUE(resA);
  EXPECT_TRUE(resB);
  EXPECT_TRUE(inspectContactPoints(s1, tf1, s2, tf2, GST_LIBCCD,
                                   contactsA, contactsB,
                                   true, true, true, false, tol));

  resA = solver2.shapeIntersect(s1, tf1, s2, tf2, &contactsA);
  resB = solver2.shapeIntersect(s2, tf2, s1, tf1, &contactsB);

  // normal should be opposite
  for (size_t i = 0; i < contactsB.size(); ++i)
    contactsB[i].normal = -contactsB[i].normal;

  EXPECT_TRUE(resA);
  EXPECT_TRUE(resB);
  EXPECT_TRUE(inspectContactPoints(s1, tf1, s2, tf2, GST_INDEP,
                                   contactsA, contactsB,
                                   true, true, true, false, tol));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, reversibleShapeIntersection_allshapes)
{
  // This test check whether a shape intersection algorithm is called for the
  // reverse case as well. For example, if FCL has sphere-capsule intersection
  // algorithm, then this algorithm should be called for capsule-sphere case.

  // Prepare all kinds of primitive shapes (8) -- box, sphere, ellipsoid, capsule, cone, cylinder, plane, halfspace
  Box box(10, 10, 10);
  Sphere sphere(5);
  Ellipsoid ellipsoid(5, 5, 5);
  Capsule capsule(5, 10);
  Cone cone(5, 10);
  Cylinder cylinder(5, 10);
  Plane plane(Vector3d::Zero(), 0.0);
  Halfspace halfspace(Vector3d::Zero(), 0.0);

  // Use sufficiently short distance so that all the primitive shapes can intersect
  FCL_REAL distance = 5.0;

  // If new shape intersection algorithm is added for two distinct primitive
  // shapes, uncomment associated lines. For example, box-sphere intersection
  // algorithm is added, then uncomment box-sphere.

//  testReversibleShapeIntersection(box, sphere, distance);
//  testReversibleShapeIntersection(box, ellipsoid, distance);
//  testReversibleShapeIntersection(box, capsule, distance);
//  testReversibleShapeIntersection(box, cone, distance);
//  testReversibleShapeIntersection(box, cylinder, distance);
  testReversibleShapeIntersection(box, plane, distance);
  testReversibleShapeIntersection(box, halfspace, distance);

//  testReversibleShapeIntersection(sphere, ellipsoid, distance);
  testReversibleShapeIntersection(sphere, capsule, distance);
//  testReversibleShapeIntersection(sphere, cone, distance);
//  testReversibleShapeIntersection(sphere, cylinder, distance);
  testReversibleShapeIntersection(sphere, plane, distance);
  testReversibleShapeIntersection(sphere, halfspace, distance);

//  testReversibleShapeIntersection(ellipsoid, capsule, distance);
//  testReversibleShapeIntersection(ellipsoid, cone, distance);
//  testReversibleShapeIntersection(ellipsoid, cylinder, distance);
//  testReversibleShapeIntersection(ellipsoid, plane, distance);
//  testReversibleShapeIntersection(ellipsoid, halfspace, distance);

//  testReversibleShapeIntersection(capsule, cone, distance);
//  testReversibleShapeIntersection(capsule, cylinder, distance);
  testReversibleShapeIntersection(capsule, plane, distance);
  testReversibleShapeIntersection(capsule, halfspace, distance);

//  testReversibleShapeIntersection(cone, cylinder, distance);
  testReversibleShapeIntersection(cone, plane, distance);
  testReversibleShapeIntersection(cone, halfspace, distance);

  testReversibleShapeIntersection(cylinder, plane, distance);
  testReversibleShapeIntersection(cylinder, halfspace, distance);

  testReversibleShapeIntersection(plane, halfspace, distance);
}

template<typename S1, typename S2>
void testReversibleShapeDistance(const S1& s1, const S2& s2, FCL_REAL distance)
{
  Transform3d tf1(Eigen::Translation3d(Vector3d(-0.5 * distance, 0.0, 0.0)));
  Transform3d tf2(Eigen::Translation3d(Vector3d(+0.5 * distance, 0.0, 0.0)));

  FCL_REAL distA;
  FCL_REAL distB;
  Vector3d p1A;
  Vector3d p1B;
  Vector3d p2A;
  Vector3d p2B;

  bool resA;
  bool resB;

  const double tol = 1e-6;

  resA = solver1.shapeDistance(s1, tf1, s2, tf2, &distA, &p1A, &p2A);
  resB = solver1.shapeDistance(s2, tf2, s1, tf1, &distB, &p1B, &p2B);

  EXPECT_TRUE(resA);
  EXPECT_TRUE(resB);
  EXPECT_NEAR(distA, distB, tol);  // distances should be same
  EXPECT_TRUE(p1A.isApprox(p2B, tol));  // closest points should in reverse order
  EXPECT_TRUE(p2A.isApprox(p1B, tol));

  resA = solver2.shapeDistance(s1, tf1, s2, tf2, &distA, &p1A, &p2A);
  resB = solver2.shapeDistance(s2, tf2, s1, tf1, &distB, &p1B, &p2B);

  EXPECT_TRUE(resA);
  EXPECT_TRUE(resB);
  EXPECT_NEAR(distA, distB, tol);
  EXPECT_TRUE(p1A.isApprox(p2B, tol));
  EXPECT_TRUE(p2A.isApprox(p1B, tol));
}

GTEST_TEST(FCL_GEOMETRIC_SHAPES, reversibleShapeDistance_allshapes)
{
  // This test check whether a shape distance algorithm is called for the
  // reverse case as well. For example, if FCL has sphere-capsule distance
  // algorithm, then this algorithm should be called for capsule-sphere case.

  // Prepare all kinds of primitive shapes (8) -- box, sphere, ellipsoid, capsule, cone, cylinder, plane, halfspace
  Box box(10, 10, 10);
  Sphere sphere(5);
  Ellipsoid ellipsoid(5, 5, 5);
  Capsule capsule(5, 10);
  Cone cone(5, 10);
  Cylinder cylinder(5, 10);
  Plane plane(Vector3d::Zero(), 0.0);
  Halfspace halfspace(Vector3d::Zero(), 0.0);

  // Use sufficiently long distance so that all the primitive shapes CANNOT intersect
  FCL_REAL distance = 15.0;

  // If new shape distance algorithm is added for two distinct primitive
  // shapes, uncomment associated lines. For example, box-sphere intersection
  // algorithm is added, then uncomment box-sphere.

//  testReversibleShapeDistance(box, sphere, distance);
//  testReversibleShapeDistance(box, ellipsoid, distance);
//  testReversibleShapeDistance(box, capsule, distance);
//  testReversibleShapeDistance(box, cone, distance);
//  testReversibleShapeDistance(box, cylinder, distance);
//  testReversibleShapeDistance(box, plane, distance);
//  testReversibleShapeDistance(box, halfspace, distance);

//  testReversibleShapeDistance(sphere, ellipsoid, distance);
  testReversibleShapeDistance(sphere, capsule, distance);
//  testReversibleShapeDistance(sphere, cone, distance);
//  testReversibleShapeDistance(sphere, cylinder, distance);
//  testReversibleShapeDistance(sphere, plane, distance);
//  testReversibleShapeDistance(sphere, halfspace, distance);

//  testReversibleShapeDistance(ellipsoid, capsule, distance);
//  testReversibleShapeDistance(ellipsoid, cone, distance);
//  testReversibleShapeDistance(ellipsoid, cylinder, distance);
//  testReversibleShapeDistance(ellipsoid, plane, distance);
//  testReversibleShapeDistance(ellipsoid, halfspace, distance);

//  testReversibleShapeDistance(capsule, cone, distance);
//  testReversibleShapeDistance(capsule, cylinder, distance);
//  testReversibleShapeDistance(capsule, plane, distance);
//  testReversibleShapeDistance(capsule, halfspace, distance);

//  testReversibleShapeDistance(cone, cylinder, distance);
//  testReversibleShapeDistance(cone, plane, distance);
//  testReversibleShapeDistance(cone, halfspace, distance);

//  testReversibleShapeDistance(cylinder, plane, distance);
//  testReversibleShapeDistance(cylinder, halfspace, distance);

//  testReversibleShapeDistance(plane, halfspace, distance);
}

//==============================================================================
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
