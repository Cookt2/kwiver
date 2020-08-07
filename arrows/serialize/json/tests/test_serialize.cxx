/*ckwg +29
 * Copyright 2018-2020 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief test json serializers
 */

#include <gtest/gtest.h>

#include <vital/types/activity.h>
#include <vital/types/bounding_box.h>
#include <vital/types/class_map.h>
#include <vital/types/detected_object.h>
#include <vital/types/timestamp.h>
#include <vital/types/image_container.h>
#include <vital/types/track_set.h>
#include <vital/types/track.h>
#include <vital/types/object_track_set.h>

#include <arrows/serialize/json/activity.h>
#include <arrows/serialize/json/bounding_box.h>
#include <arrows/serialize/json/class_map.h>
#include <arrows/serialize/json/detected_object.h>
#include <arrows/serialize/json/detected_object_set.h>
#include <arrows/serialize/json/timestamp.h>
#include <arrows/serialize/json/image.h>
#include <arrows/serialize/json/string.h>
#include <arrows/serialize/json/track_state.h>
#include <arrows/serialize/json/track.h>
#include <arrows/serialize/json/object_track_state.h>
#include <arrows/serialize/json/track_set.h>
#include <arrows/serialize/json/object_track_set.h>

#include <vital/util/string.h>

#include <vital/internal/cereal/types/polymorphic.hpp>
#include <iostream>
namespace kasj = kwiver::arrows::serialize::json;

// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}

// ----------------------------------------------------------------------------
TEST( serialize, activity_default )
{
  // This tests the behavior when participants
  // and class_map are set to NULL
  auto const act = kwiver::vital::activity{};
  auto act_ser = kasj::activity{};

  auto const mes = act_ser.serialize( kwiver::vital::any( act ) );

  auto const dser = act_ser.deserialize( *mes );
  auto const act_dser =
    kwiver::vital::any_cast< kwiver::vital::activity >( dser );

  // Check members
  EXPECT_EQ( act.id(), act_dser.id() );
  EXPECT_EQ( act.label(), act_dser.label() );
  EXPECT_EQ( act.activity_type(), act_dser.activity_type() );
  EXPECT_EQ( act.participants(), act_dser.participants() );
  EXPECT_DOUBLE_EQ( act.confidence(), act_dser.confidence() );

  // Timestamps are invalid so can't do a direct comparison
  auto const start = act.start();
  auto const end = act.end();
  auto const start_dser = act_dser.start();
  auto const end_dser = act_dser.end();

  EXPECT_EQ( start.get_time_seconds(), start_dser.get_time_seconds() );
  EXPECT_EQ( start.get_frame(), start_dser.get_frame() );
  EXPECT_EQ( start.get_time_domain_index(), start_dser.get_time_domain_index() );

  EXPECT_EQ( end.get_time_seconds(), end_dser.get_time_seconds() );
  EXPECT_EQ( end.get_frame(), end_dser.get_frame() );
  EXPECT_EQ( end.get_time_domain_index(), end_dser.get_time_domain_index() );
}

// ----------------------------------------------------------------------------
TEST( serialize, activity )
{
  auto cm_sptr = std::make_shared< kwiver::vital::class_map >();
  cm_sptr->set_score( "first", 1 );
  cm_sptr->set_score( "second", 10 );
  cm_sptr->set_score( "third", 101 );

  // Create object_track_set consisting of
  // 1 track_sptr with 10 track states
  auto track_sptr = kwiver::vital::track::create();
  track_sptr->set_id( 1 );
  for ( int i = 0; i < 10; i++ )
  {
    auto const bbox =
      kwiver::vital::bounding_box_d{ 10.0 + i, 10.0 + i, 20.0 + i, 20.0 + i };

    auto dobj_cm_sptr = std::make_shared< kwiver::vital::class_map >();
    dobj_cm_sptr->set_score( "key", i / 10.0 );

    auto const dobj_sptr =
      std::make_shared< kwiver::vital::detected_object >( bbox, i / 10.0, dobj_cm_sptr );

    auto const ots_sptr =
      std::make_shared< kwiver::vital::object_track_state >( i, i, dobj_sptr );

    track_sptr->append( ots_sptr );
  }

  auto const tracks = std::vector< kwiver::vital::track_sptr >{ track_sptr };
  auto const obj_trk_set_sptr =
    std::make_shared< kwiver::vital::object_track_set >( tracks );

  // Now both timestamps
  auto const start = kwiver::vital::timestamp{ 1, 1 };
  auto const end = kwiver::vital::timestamp{ 2, 2 };

  // Now construct activity
  auto const act =
    kwiver::vital::activity{ 5, "test_label", 3.1415, cm_sptr, start, end, obj_trk_set_sptr };

  auto act_ser = kasj::activity{};

  auto const mes = act_ser.serialize( kwiver::vital::any( act ) );

  auto const dser = act_ser.deserialize( *mes );
  auto const act_dser =
    kwiver::vital::any_cast< kwiver::vital::activity >( dser );

  // Now check equality
  EXPECT_EQ( act.id(), act_dser.id() );
  EXPECT_EQ( act.label(), act_dser.label() );
  EXPECT_DOUBLE_EQ( act.confidence(), act_dser.confidence() );
  EXPECT_EQ( act.start(), act_dser.start() );
  EXPECT_EQ( act.end(), act_dser.end() );

  // Check values in the retrieved class map
  auto const act_type = act.activity_type();
  auto const act_type_dser = act_dser.activity_type();
  EXPECT_EQ( act_type->size(), act_type_dser->size() );
  EXPECT_DOUBLE_EQ( act_type->score( "first" ),  act_type_dser->score( "first" ) );
  EXPECT_DOUBLE_EQ( act_type->score( "second" ), act_type_dser->score( "second" ) );
  EXPECT_DOUBLE_EQ( act_type->score( "third" ),  act_type_dser->score( "third" ) );

  // Now the object_track_set
  auto const parts = act.participants();
  auto const parts_dser = act_dser.participants();

  EXPECT_EQ( parts->size(), parts_dser->size() );

  auto const trk = parts->get_track( 1 );
  auto const trk_dser = parts_dser->get_track( 1 );

  // Iterate over the track_states
  for ( int i = 0; i < 10; i++ )
  {
    auto const trk_state_sptr = *trk->find( i );
    auto const trk_state_dser_sptr = *trk_dser->find( i );

    EXPECT_EQ( trk_state_sptr->frame(), trk_state_dser_sptr->frame() );

    auto const obj_trk_state_sptr =
      kwiver::vital::object_track_state::downcast( trk_state_sptr );
    auto const obj_trk_state_dser_sptr =
      kwiver::vital::object_track_state::downcast( trk_state_dser_sptr );

    EXPECT_EQ( obj_trk_state_sptr->time(), obj_trk_state_dser_sptr->time() );

    auto const do_ser_sptr = obj_trk_state_sptr->detection();
    auto const do_dser_sptr = obj_trk_state_dser_sptr->detection();

    EXPECT_EQ( do_ser_sptr->bounding_box(), do_dser_sptr->bounding_box() );
    EXPECT_EQ( do_ser_sptr->confidence(), do_dser_sptr->confidence() );

    auto const cm_ser_sptr = do_ser_sptr->type();
    auto const cm_dser_sptr = do_dser_sptr->type();

    if ( cm_ser_sptr )
    {
      EXPECT_EQ( cm_ser_sptr->size(), cm_dser_sptr->size() );
      EXPECT_EQ( cm_ser_sptr->score( "key" ), cm_dser_sptr->score( "key" ) );
    }
  }
}

// ----------------------------------------------------------------------------
TEST( serialize, bounding_box )
{
  kasj::bounding_box bbox_ser;
  kwiver::vital::bounding_box_d bbox { 1, 2, 3, 4 };

  kwiver::vital::any bb_any( bbox );
  auto mes = bbox_ser.serialize( bb_any );

  // std::cout << "Serialized bbox: \"" << *mes << "\"\n";
  // std::cout << "List of element names: " << kwiver::vital::join( names, ", " ) << std::endl;

  auto dser = bbox_ser.deserialize( *mes );
  kwiver::vital::bounding_box_d bbox_dser =
    kwiver::vital::any_cast< kwiver::vital::bounding_box_d >( dser );

  /* useful for debugging
  std::cout << "bbox_dser { " << bbox_dser.min_x() << ", "
            << bbox_dser.min_y() << ", "
            << bbox_dser.max_x() << ", "
            << bbox_dser.max_y() << "}\n";
  */

  EXPECT_EQ( bbox, bbox_dser );
}

// ----------------------------------------------------------------------------
TEST( serialize, class_map )
{
  kasj::class_map cm_ser; // get serializer
  kwiver::vital::class_map cm;

  cm.set_score( "first", 1 );
  cm.set_score( "second", 10 );
  cm.set_score( "third", 101 );
  cm.set_score( "last", 121 );

  kwiver::vital::any cm_any( cm );
  auto mes = cm_ser.serialize( cm_any );

  // useful for debugging
  // std::cout << "Serialized cm: \"" << *mes << "\"\n";

  auto dser = cm_ser.deserialize( *mes );
  kwiver::vital::class_map cm_dser =
    kwiver::vital::any_cast< kwiver::vital::class_map >( dser );

  EXPECT_EQ( cm.size(), cm_dser.size() );

  auto o_it = cm.begin();
  auto d_it = cm_dser.begin();

  for (size_t i = 0; i < cm.size(); ++i )
  {
    EXPECT_EQ( *(o_it->first), *(d_it->first) );
    EXPECT_EQ( o_it->second, d_it->second );
    ++o_it;
    ++d_it;
  }
}

// ----------------------------------------------------------------------------
TEST( serialize, detected_object )
{
  kasj::detected_object obj_ser; // get serializer

  auto cm = std::make_shared<kwiver::vital::class_map>();

  cm->set_score( "first", 1 );
  cm->set_score( "second", 10 );
  cm->set_score( "third", 101 );
  cm->set_score( "last", 121 );

  auto obj = std::make_shared< kwiver::vital::detected_object>(
    kwiver::vital::bounding_box_d{ 1, 2, 3, 4 }, 3.14159, cm );
  obj->set_detector_name( "test_detector" );
  obj->set_index( 1234 );

  kwiver::vital::any obj_any( obj );
  auto mes = obj_ser.serialize( obj_any );

  // useful for debugging
  // std::cout << "Serialized cm: \"" << *mes << "\"\n";

  auto dser = obj_ser.deserialize( *mes );
  auto obj_dser = kwiver::vital::any_cast< kwiver::vital::detected_object_sptr >( dser );

  EXPECT_EQ( obj->bounding_box(), obj_dser->bounding_box() );
  EXPECT_EQ( obj->index(), obj_dser->index() );
  EXPECT_EQ( obj->confidence(), obj_dser->confidence() );
  EXPECT_EQ( obj->detector_name(), obj_dser->detector_name() );

  // Notes
  {
    auto obj_notes = obj->notes();
    auto dser_notes = obj_dser->notes();
    EXPECT_EQ( obj_notes.size(), dser_notes.size() );
    for ( size_t i = 0; i < obj_notes.size(); ++i )
    {
      EXPECT_EQ( obj_notes[i], dser_notes[i] );
    }
  }

  // keypoints
  {
    auto obj_kp = obj->keypoints();
    auto dser_kp = obj_dser->keypoints();
    EXPECT_EQ( obj_kp.size(), dser_kp.size() );
    EXPECT_EQ( obj_kp, dser_kp );
  }

  // detected object type
  cm = obj->type();
  if (cm)
  {
    auto cm_dser = obj_dser->type();

    EXPECT_EQ( cm->size(), cm_dser->size() );

    auto o_it = cm->begin();
    auto d_it = cm_dser->begin();

    for (size_t i = 0; i < cm->size(); ++i )
    {
      EXPECT_EQ( *(o_it->first), *(d_it->first) );
      EXPECT_EQ( o_it->second, d_it->second );
      ++o_it;
      ++d_it;
    }
  }
}


// ----------------------------------------------------------------------------
TEST( serialize, detected_object_set )
{
  kasj::detected_object_set obj_ser; // get serializer
  kwiver::vital::detected_object_set_sptr dos =
    std::make_shared<kwiver::vital::detected_object_set>();;
  auto cm = std::make_shared<kwiver::vital::class_map>();

  cm->set_score( "first", 1 );
  cm->set_score( "second", 10 );
  cm->set_score( "third", 101 );
  cm->set_score( "last", 121 );

  auto det_obj = std::make_shared< kwiver::vital::detected_object>(
    kwiver::vital::bounding_box_d{ 1, 2, 3, 4 }, 3.14159, cm );
  det_obj->set_detector_name( "test_detector" );
  det_obj->set_index( 1234 );

  dos->add( det_obj );
  dos->add( det_obj );
  dos->add( det_obj );

  kwiver::vital::any obj_any( dos );
  auto mes = obj_ser.serialize( obj_any );

  // Useful for debugging
  // std::cout << "Serialized dos: \"" << *mes << "\"\n";

  auto dser = obj_ser.deserialize( *mes );
  auto obj_dser_set = kwiver::vital::any_cast< kwiver::vital::detected_object_set_sptr >( dser );

  EXPECT_EQ( 3, obj_dser_set->size() );

  kwiver::vital::detected_object_set::const_iterator ei = obj_dser_set->cend();
  kwiver::vital::detected_object_set::const_iterator obj_dser;

  for ( obj_dser = obj_dser_set->cbegin(); obj_dser != ei; ++obj_dser )
  {
    EXPECT_EQ( det_obj->bounding_box(), (*obj_dser)->bounding_box() );
    EXPECT_EQ( det_obj->index(), (*obj_dser)->index() );
    EXPECT_EQ( det_obj->confidence(), (*obj_dser)->confidence() );
    EXPECT_EQ( det_obj->detector_name(), (*obj_dser)->detector_name() );

    cm = det_obj->type();
    if (cm)
    {
      auto cm_dser = (*obj_dser)->type();

      EXPECT_EQ( cm->size(), cm_dser->size() );

      auto o_it = cm->begin();
      auto d_it = cm_dser->begin();

      for (size_t i = 0; i < cm->size(); ++i )
      {
        EXPECT_EQ( *(o_it->first), *(d_it->first) );
        EXPECT_EQ( o_it->second, d_it->second );
        ++o_it;
        ++d_it;
      }
    }
  }
}

// ----------------------------------------------------------------------------
TEST( serialize, timestamp)
{
  kasj::timestamp tstamp_ser;
  kwiver::vital::timestamp tstamp{1, 1};

  kwiver::vital::any tstamp_any( tstamp );

  auto mes = tstamp_ser.serialize( tstamp_any );
  auto dser = tstamp_ser.deserialize ( *mes );

  kwiver::vital::timestamp tstamp_dser =
    kwiver::vital::any_cast< kwiver::vital::timestamp >( dser );

  EXPECT_EQ( tstamp, tstamp_dser);
}

// ----------------------------------------------------------------------------
TEST( serialize, image)
{
  kasj::image image_ser;
  kwiver::vital::image img{200, 300, 3};

  char* cp = static_cast< char* >(img.memory()->data() );
  for ( size_t i = 0; i < img.size(); ++i )
  {
    *cp++ = i;
  }
  {
    kwiver::vital::image_container_sptr img_container =
      std::make_shared< kwiver::vital::simple_image_container >( img );
    kwiver::vital::any img_any(img_container);

    auto mes = image_ser.serialize( img_any );
    auto dser = image_ser.deserialize( *mes );

    auto img_dser = kwiver::vital::any_cast< kwiver::vital::image_container_sptr > ( dser );

    // Check the content of images
    EXPECT_TRUE( kwiver::vital::equal_content( img_container->get_image(), img_dser->get_image()) );
  }


  {
    kwiver::vital::image other( img.memory(),
                                ((char *)img.first_pixel() + 32),
                                100, 200, img.depth(),
                                img.w_step(), img.h_step(), img.d_step(),
                                img.pixel_traits() );

    kwiver::vital::image_container_sptr img_container =
      std::make_shared< kwiver::vital::simple_image_container >( other );
    kwiver::vital::any img_any(img_container);

    auto mes = image_ser.serialize( img_any );
    auto dser = image_ser.deserialize( *mes );

    auto img_dser = kwiver::vital::any_cast< kwiver::vital::image_container_sptr > ( dser );

    // Check the content of images
    EXPECT_TRUE( kwiver::vital::equal_content( img_container->get_image(), img_dser->get_image()) );
  }

  {
    kwiver::vital::image other( img.memory(),
                                ((char *)img.first_pixel() + (3 * img.width()) ),
                                img.width(), 200, img.depth(),
                                img.w_step(), img.h_step(), img.d_step(),
                                img.pixel_traits() );

    kwiver::vital::image_container_sptr img_container =
      std::make_shared< kwiver::vital::simple_image_container >( other );
    kwiver::vital::any img_any(img_container);

    auto mes = image_ser.serialize( img_any );
    auto dser = image_ser.deserialize( *mes );

    auto img_dser = kwiver::vital::any_cast< kwiver::vital::image_container_sptr > ( dser );

    // Check the content of images
    EXPECT_TRUE( kwiver::vital::equal_content( img_container->get_image(), img_dser->get_image()) );
  }
}

// ----------------------------------------------------------------------------
TEST (serialize, string)
{
  kasj::string str_ser;
  std::string str("Test string");

  kwiver::vital::any str_any(str);

  auto mes = str_ser.serialize( str_any );
  auto dser = str_ser.deserialize( *mes );

  std::string str_dser =
    kwiver::vital::any_cast< std::string > ( dser );

  // std::cout << tstamp_dser.pretty_print() << std::endl;

  EXPECT_EQ (str, str_dser);
}
// ----------------------------------------------------------------------------

TEST( serialize, track_state)
{
  kasj::track_state trk_state_ser;
  kwiver::vital::track_state trk_state{1};
  kwiver::vital::any trk_state_any( trk_state );

  auto mes = trk_state_ser.serialize( trk_state_any );
  auto dser = trk_state_ser.deserialize( *mes );

  kwiver::vital::track_state trk_state_dser = kwiver::vital::any_cast<
                                kwiver::vital::track_state >( dser );

  EXPECT_EQ( trk_state.frame(), trk_state_dser.frame() );
}

// ----------------------------------------------------------------------------
TEST( serialize, object_track_state)
{
  auto cm = std::make_shared<kwiver::vital::class_map>();

  cm->set_score( "first", 1 );
  cm->set_score( "second", 10 );
  cm->set_score( "third", 101 );
  cm->set_score( "last", 121 );

  auto obj = std::make_shared< kwiver::vital::detected_object>(
    kwiver::vital::bounding_box_d{ 1, 2, 3, 4 }, 3.14159, cm );
  obj->set_detector_name( "test_detector" );
  obj->set_index( 1234 );


  kwiver::vital::object_track_state obj_trk_state(10, 32, obj);
  kwiver::vital::any obj_trk_state_any( obj_trk_state );
  kasj::object_track_state obj_trk_state_ser;

  auto mes = obj_trk_state_ser.serialize( obj_trk_state );

  auto dser = obj_trk_state_ser.deserialize( *mes );

  kwiver::vital::object_track_state obj_dser = kwiver::vital::any_cast<
                                  kwiver::vital::object_track_state > ( dser );

  auto do_sptr = obj_trk_state.detection();
  auto do_sptr_dser = obj_dser.detection();

  EXPECT_EQ( do_sptr->bounding_box(), do_sptr_dser->bounding_box() );
  EXPECT_EQ( do_sptr->index(), do_sptr_dser->index() );
  EXPECT_EQ( do_sptr->confidence(), do_sptr_dser->confidence() );
  EXPECT_EQ( do_sptr->detector_name(), do_sptr_dser->detector_name() );

  auto cm_sptr_dser = do_sptr_dser->type();

  if ( cm )
  {
    EXPECT_EQ( cm->size(), cm_sptr_dser->size() );

    auto it = cm->begin();
    auto it_dser = cm_sptr_dser->begin();

    for ( size_t i = 0; i < cm->size(); ++i )
    {
      EXPECT_EQ( *(it->first), *(it_dser->first) );
      EXPECT_EQ( it->second, it_dser->second );
    }
  }
  EXPECT_EQ(obj_trk_state.time(), obj_dser.time());
  EXPECT_EQ(obj_trk_state.frame(), obj_dser.frame());
}
// ----------------------------------------------------------------------------

TEST(serialize , track )
{

  // test track with object track state
  auto obj_trk = kwiver::vital::track::create();
  obj_trk->set_id(1);
  for (int i=0; i<1; i++)
  {
    auto cm = std::make_shared<kwiver::vital::class_map>();

    cm->set_score( "first", 1 );
    cm->set_score( "second", 10 );
    cm->set_score( "third", 101 );
    cm->set_score( "last", 121 );

    auto dobj_sptr = std::make_shared< kwiver::vital::detected_object>(
                            kwiver::vital::bounding_box_d{ 1, 2, 3, 4 },
                                3.14159265, cm );
    dobj_sptr->set_detector_name( "test_detector" );
    dobj_sptr->set_index( 1234 );
    auto obj_trk_state_sptr = std::make_shared< kwiver::vital::object_track_state >
                                ( i, i, dobj_sptr );

    bool insert_success = obj_trk->insert( obj_trk_state_sptr );
    if ( !insert_success )
    {
      std::cerr << "Failed to insert object track state" << std::endl;
    }
  }

  kasj::track obj_trk_ser;
  kwiver::vital::any obj_trk_any( obj_trk );

  auto mes = obj_trk_ser.serialize( obj_trk_any );

  auto dser = obj_trk_ser.deserialize( *mes );

  auto obj_trk_dser = kwiver::vital::any_cast< kwiver::vital::track_sptr >( dser );

  // Check track id
  EXPECT_EQ( obj_trk->id(), obj_trk_dser->id() );

  for ( int i=0; i<1; i++ )
  {
    auto trk_state_sptr = *obj_trk->find( i );
    auto dser_trk_state_sptr = *obj_trk_dser->find( i );

    EXPECT_EQ( trk_state_sptr->frame(), dser_trk_state_sptr->frame() );

    auto obj_trk_state_sptr = kwiver::vital::object_track_state::downcast( trk_state_sptr );
    auto dser_obj_trk_state_sptr = kwiver::vital::object_track_state::
                                                    downcast( dser_trk_state_sptr );

    //std::cout << dser_obj_trk_state_sptr << std::endl;

    auto ser_do_sptr = obj_trk_state_sptr->detection();
    auto dser_do_sptr = dser_obj_trk_state_sptr->detection();

    EXPECT_EQ( ser_do_sptr->bounding_box(), dser_do_sptr->bounding_box() );
    EXPECT_EQ( ser_do_sptr->index(), dser_do_sptr->index() );
    EXPECT_EQ( ser_do_sptr->confidence(), dser_do_sptr->confidence() );
    EXPECT_EQ( ser_do_sptr->detector_name(), dser_do_sptr->detector_name() );

    auto ser_cm_sptr = ser_do_sptr->type();
    auto dser_cm_sptr = dser_do_sptr->type();

    if ( ser_cm_sptr )
    {
      EXPECT_EQ( ser_cm_sptr->size(),dser_cm_sptr->size() );

      auto ser_it = ser_cm_sptr->begin();
      auto dser_it = dser_cm_sptr->begin();

      for ( size_t i = 0; i < ser_cm_sptr->size(); ++i )
      {
        EXPECT_EQ( *(ser_it->first), *(ser_it->first) );
        EXPECT_EQ( dser_it->second, dser_it->second );
      }
    }
  }

  // Test with track state

  auto trk = kwiver::vital::track::create();
  trk->set_id( 2 );
  for ( int i=0; i<10; i++ )
  {
    auto trk_state_sptr = std::make_shared< kwiver::vital::track_state>( i );
    bool insert_success = trk->insert( trk_state_sptr );
    if ( !insert_success )
    {
      std::cerr << "Failed to insert track state" << std::endl;
    }
  }

  kasj::track trk_ser;
  kwiver::vital::any trk_any( trk );

  auto mes_trk = trk_ser.serialize( trk_any );

  auto dser_trk = trk_ser.deserialize( *mes_trk );

  auto trk_dser = kwiver::vital::any_cast< kwiver::vital::track_sptr >( dser_trk );

  EXPECT_EQ( trk->id(), trk_dser->id() );

  for ( int i=0; i<10; i++ )
  {
    auto trk_state_sptr = *trk->find( i );
    auto dser_trk_state_sptr = *trk_dser->find( i );

    EXPECT_EQ( trk_state_sptr->frame(), dser_trk_state_sptr->frame() );
  }
}

// ============================================================================
TEST( serialize, track_set )
{
  auto trk_set_sptr = std::make_shared< kwiver::vital::track_set >();
  for ( kwiver::vital::track_id_t trk_id=1; trk_id<5; ++trk_id )
  {
    auto trk = kwiver::vital::track::create();
    trk->set_id( trk_id );

    for ( int i=trk_id*10; i < ( trk_id+1 )*10; i++ )
    {
      auto trk_state_sptr = std::make_shared< kwiver::vital::track_state>( i );
      bool insert_success = trk->insert( trk_state_sptr );
      if ( !insert_success )
      {
        std::cerr << "Failed to insert track state" << std::endl;
      }
    }
    trk_set_sptr->insert(trk);
  }
  kasj::track_set trk_set_ser;
  kwiver::vital::any trk_state_any( trk_set_sptr );
  auto msg_trk_set = trk_set_ser.serialize( trk_state_any );
  auto dser_trk = trk_set_ser.deserialize( *msg_trk_set );
  auto trk_set_sptr_dser = kwiver::vital::any_cast< kwiver::vital::track_set_sptr >( dser_trk );

  for ( kwiver::vital::track_id_t trk_id=1; trk_id<5; ++trk_id )
  {
    auto trk = trk_set_sptr->get_track( trk_id );
    auto trk_dser = trk_set_sptr_dser->get_track( trk_id );
    EXPECT_EQ( trk->id(), trk_dser->id() );
    for ( int i=trk_id*10; i < ( trk_id+1 )*10; i++ )
    {
      auto obj_trk_state_sptr = *trk->find( i );
      auto dser_trk_state_sptr = *trk_dser->find( i );

      EXPECT_EQ( obj_trk_state_sptr->frame(), dser_trk_state_sptr->frame() );
    }
  }
}
// ============================================================================
TEST( serialize, object_track_set )
{
  auto obj_trk_set_sptr = std::make_shared< kwiver::vital::object_track_set >();
  for ( kwiver::vital::track_id_t trk_id=1; trk_id<3; ++trk_id )
  {
    auto trk = kwiver::vital::track::create();
    trk->set_id( trk_id );
    for ( int i=trk_id*2; i < ( trk_id+1 )*2; i++ )
    {
      auto cm = std::make_shared<kwiver::vital::class_map>();

      cm->set_score( "first", 1 );
      cm->set_score( "second", 10 );
      cm->set_score( "third", 101 );
      cm->set_score( "last", 121 );

      auto dobj_sptr = std::make_shared< kwiver::vital::detected_object>(
                              kwiver::vital::bounding_box_d{ 1, 2, 3, 4 },
                                  3.14159265, cm );
      dobj_sptr->set_detector_name( "test_detector" );
      dobj_sptr->set_index( 1234 );
      auto obj_trk_state_sptr = std::make_shared< kwiver::vital::object_track_state >
                                  ( i, i, dobj_sptr );

      bool insert_success = trk->insert( obj_trk_state_sptr );
      if ( !insert_success )
      {
        std::cerr << "Failed to insert object track state" << std::endl;
      }
    }
    obj_trk_set_sptr->insert(trk);
  }

  kasj::object_track_set obj_trk_set_ser;
  kwiver::vital::any obj_trk_state_any( obj_trk_set_sptr );
  auto msg_obj_trk_set = obj_trk_set_ser.serialize( obj_trk_state_any );
  auto dser_obj_trk = obj_trk_set_ser.deserialize( *msg_obj_trk_set );
  auto obj_trk_set_sptr_dser =
      kwiver::vital::any_cast< kwiver::vital::object_track_set_sptr >( dser_obj_trk );
  for ( kwiver::vital::track_id_t trk_id=1; trk_id<3; ++trk_id )
  {
    auto trk = obj_trk_set_sptr->get_track( trk_id );
    auto trk_dser = obj_trk_set_sptr_dser->get_track( trk_id );
    EXPECT_EQ( trk->id(), trk_dser->id() );
    for ( int i=trk_id*2; i < ( trk_id+1 )*2; i++ )
    {
      auto trk_state_sptr = *trk->find( i );
      auto dser_trk_state_sptr = *trk_dser->find( i );

      EXPECT_EQ( trk_state_sptr->frame(), dser_trk_state_sptr->frame() );
      auto obj_trk_state_sptr = kwiver::vital::object_track_state::downcast( trk_state_sptr );
      auto dser_obj_trk_state_sptr = kwiver::vital::object_track_state::
                                                      downcast( dser_trk_state_sptr );


      auto ser_do_sptr = obj_trk_state_sptr->detection();
      auto dser_do_sptr = dser_obj_trk_state_sptr->detection();

      EXPECT_EQ( ser_do_sptr->bounding_box(), dser_do_sptr->bounding_box() );
      EXPECT_EQ( ser_do_sptr->index(), dser_do_sptr->index() );
      EXPECT_EQ( ser_do_sptr->confidence(), dser_do_sptr->confidence() );
      EXPECT_EQ( ser_do_sptr->detector_name(), dser_do_sptr->detector_name() );

      auto ser_cm_sptr = ser_do_sptr->type();
      auto dser_cm_sptr = dser_do_sptr->type();

      if ( ser_cm_sptr )
      {
        EXPECT_EQ( ser_cm_sptr->size(),dser_cm_sptr->size() );

        auto ser_it = ser_cm_sptr->begin();
        auto dser_it = dser_cm_sptr->begin();

        for ( size_t i = 0; i < ser_cm_sptr->size(); ++i )
        {
          EXPECT_EQ( *(ser_it->first), *(ser_it->first) );
          EXPECT_EQ( dser_it->second, dser_it->second );
        }
      }
    }
  }
}
