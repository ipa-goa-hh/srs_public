/******************************************************************************
 * \file
 *
 * $Id:$
 *
 * Copyright (C) Brno University of Technology
 *
 * This file is part of software developed by dcgm-robotics@FIT group.
 *
 * Author: Zdenek Materna (imaterna@fit.vutbr.cz)
 * Supervised by: Michal Spanel (spanel@fit.vutbr.cz)
 * Date: 5/4/2012
 * 
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * @todo Add timeout to ArmNavNew message. Show current timeout to user.
 * @todo Add button for move the gripper to selected pre-grasp position.
 * @todo Add messagebox which will be shown on finish of trajectory execution
 * @todo Add ID of reached trajectory to the ArmNavFinish
 */

#include "but_arm_manipulation.h"
#include <rviz/window_manager_interface.h>

using namespace std;

const int ID_BUTTON_NEW(101);
const int ID_BUTTON_PLAN(102);
const int ID_BUTTON_PLAY(103);
const int ID_BUTTON_EXECUTE(104);
const int ID_BUTTON_RESET(105);
const int ID_BUTTON_SUCCESS(106);
const int ID_BUTTON_FAILED(107);
const int ID_BUTTON_AUTOADJ(108);
const int ID_BUTTON_GRIPPER_O(109);
const int ID_BUTTON_GRIPPER_C(110);
const int ID_BUTTON_LOOK(111);
const int ID_BUTTON_REFRESH(112);
const int ID_BUTTON_SWITCH(113);
const int ID_BUTTON_REPEAT(114);


//DEFINE_EVENT_TYPE( C_UPDATE_GUI )

/**
 Constructor
 */
CArmManipulationControls::CArmManipulationControls(wxWindow *parent, const wxString& title, rviz::WindowManagerInterface * wmi )
    : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxSize(280, 180), wxTAB_TRAVERSAL, title)
    , m_wmi( wmi )
{
    // Create controls
    //m_button = new wxButton(this, ID_RESET_BUTTON, wxT("Reset map"));

    parent_ = parent;
    
    ros::param::param<bool>("~wait_for_start", wait_for_start_ , WAIT_FOR_START);

    m_button_new = new wxButton(this, ID_BUTTON_NEW, wxT("New"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_plan = new wxButton(this, ID_BUTTON_PLAN, wxT("Plan"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_play = new wxButton(this, ID_BUTTON_PLAY, wxT("Play"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_execute = new wxButton(this, ID_BUTTON_EXECUTE, wxT("Execute"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_reset = new wxButton(this, ID_BUTTON_RESET, wxT("Reset"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);

    m_button_autoadj = new wxButton(this, ID_BUTTON_AUTOADJ, wxT("Adjust"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_gripper_o = new wxButton(this, ID_BUTTON_GRIPPER_O, wxT("Open gripper"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_gripper_c = new wxButton(this, ID_BUTTON_GRIPPER_C, wxT("Close gripper"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_look_around = new wxButton(this, ID_BUTTON_LOOK, wxT("Look around"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_refresh = new wxButton(this, ID_BUTTON_REFRESH, wxT("Refresh"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);

    m_button_switch = new wxButton(this, ID_BUTTON_SWITCH, wxT("Disable ACO"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);

    m_button_success = new wxButton(this, ID_BUTTON_SUCCESS, wxT("Success"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_failed = new wxButton(this, ID_BUTTON_FAILED, wxT("Failed"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
    m_button_repeat = new wxButton(this, ID_BUTTON_REPEAT, wxT("Repeat"),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);

    m_text_status = new wxStaticText(this, -1, wxT("status: waiting"));
    m_text_object = new wxStaticText(this, -1, wxT("object: none"));
    m_text_timeout = new wxStaticText(this, -1, wxT("timeout: none"));
    m_text_dist = new wxStaticText(this, -1, wxT("closest pos.: none"));

    if (wait_for_start_) m_button_new->Enable(false);
    else m_button_new->Enable(true);
    m_button_plan->Enable(false);
    m_button_play->Enable(false);
    m_button_execute->Enable(false);
    m_button_reset->Enable(false);

    m_button_success->Enable(false);
    m_button_repeat->Enable(false);
    m_button_failed->Enable(false);

    m_button_switch->Enable(true);

    m_button_autoadj->Enable(false);

    m_button_gripper_o->Enable(true); // TODO povolit tlacitka jen pokud bude k dispozici actionserver?
    m_button_gripper_c->Enable(true);
    m_button_look_around->Enable(true);
    m_button_refresh->Enable(true);

    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL); // top sizer


    wxSizer *vsizer_top = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Trajectory planning"));
    wxSizer *hsizer_traj_top = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *hsizer_traj_mid = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *hsizer_traj_bot = new wxBoxSizer(wxHORIZONTAL);


    wxSizer *vsizer_mes = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Messages"));

    wxSizer *vsizer_add = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Additional controls"));

    wxSizer *hsizer_add_top = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *hsizer_add_bot = new wxBoxSizer(wxHORIZONTAL);


    /* Trajectory planning related buttons, on top*/
    hsizer_traj_top->Add(m_button_new, ID_BUTTON_NEW);
    hsizer_traj_top->Add(m_button_plan, ID_BUTTON_PLAN);
    hsizer_traj_top->Add(m_button_execute, ID_BUTTON_EXECUTE);

    hsizer_traj_mid->Add(m_button_play, ID_BUTTON_PLAY);
    hsizer_traj_mid->Add(m_button_autoadj, ID_BUTTON_AUTOADJ);
    hsizer_traj_mid->Add(m_button_reset, ID_BUTTON_RESET);

    hsizer_traj_bot->Add(m_button_success, ID_BUTTON_SUCCESS);
    hsizer_traj_bot->Add(m_button_repeat, ID_BUTTON_REPEAT);
    hsizer_traj_bot->Add(m_button_failed, ID_BUTTON_FAILED);

    vsizer_top->Add(hsizer_traj_top,1,wxEXPAND);
    vsizer_top->Add(hsizer_traj_mid,1,wxEXPAND);
    vsizer_top->Add(hsizer_traj_bot,1,wxEXPAND);


    /* Status messages*/
    vsizer_mes->Add(m_text_status);
    vsizer_mes->Add(m_text_object);
    vsizer_mes->Add(m_text_timeout);
    vsizer_mes->Add(m_text_dist);

    hsizer_add_top->Add(m_button_gripper_o);
    hsizer_add_top->Add(m_button_gripper_c);
    hsizer_add_bot->Add(m_button_look_around);
    hsizer_add_bot->Add(m_button_refresh);
    hsizer_add_bot->Add(m_button_switch);

    vsizer_add->Add(hsizer_add_top,1,wxEXPAND);
    vsizer_add->Add(hsizer_add_bot,1,wxEXPAND);

    vsizer->Add(vsizer_top,0,wxEXPAND);
    vsizer->Add(vsizer_add,0,wxEXPAND);
    vsizer->Add(vsizer_mes,0,wxEXPAND);

    // TODO: co s temi ID pozic???

    goal_pregrasp = false;
    goal_away = false;

    // TODO make it configurable - read same parameter from here and from but_arm_manip_node...
    aco_ = true;

    if (aco_) m_button_switch->SetLabel(wxT("ACO enabled"));
    else m_button_switch->SetLabel(wxT("ACO disabled"));


    vsizer->SetSizeHints(this);
    this->SetSizerAndFit(vsizer);

    ros::NodeHandle nh;

    /**
     * Service provided by plugin. It can be used to inform user that his/her action is required.
     */
    service_start_ = nh.advertiseService(SRV_START,&CArmManipulationControls::nav_start,this);

    cob_script = new cob_client("/script_server",true);

    cob_script_inited = false;



}
///////////////////////////////////////////////////////////////////////////////


CArmManipulationControls::~CArmManipulationControls() {

  if (cob_script!=NULL) delete cob_script;

  // TODO delete vsech tlacitek apod.... ??
  delete m_button_new;
  delete m_button_plan;
  delete m_button_play;
  delete m_button_execute;
  delete m_button_reset;
  delete m_button_success;
  delete m_button_failed;
  delete m_button_autoadj;
  delete m_button_gripper_o;
  delete m_button_gripper_c;
  delete m_button_look_around;
  delete m_button_refresh;
  delete m_text_status;
  delete m_text_object;
  delete m_text_timeout;
  delete m_text_dist;
  delete m_button_switch;
  delete m_button_repeat;

}

void CArmManipulationControls::NewThread() {

  ROS_INFO("Request for new trajectory");

  srs_assisted_arm_navigation::ArmNavNew srv;
  std::string status = "";
  bool success = false;

   if (ros::service::call(SRV_NEW,srv) ) {

     if (srv.response.completed) {

         success = true;

         if (goal_pregrasp) status = "status: Move arm to desired position.";
         if (goal_away) status = "status: Move arm to safe position.";

     } else {

       status = "status: " + srv.response.error;

     }

   } else {

     ROS_ERROR("Failed when calling arm_nav_new service");
     status = "status: Communication error.";

   }

   wxMutexGuiEnter();

   m_text_status->SetLabel(wxString::FromAscii(status.c_str()));

   if (success) {

     m_button_plan->Enable(true);
     m_button_reset->Enable(true);

     if (goal_pregrasp) m_button_repeat->Enable(true);

     m_button_failed->Enable(true);


   } else {

     m_button_new->Enable(true);
     if (goal_pregrasp) m_button_repeat->Enable(true);
     m_button_failed->Enable(true);

   }

   wxMutexGuiLeave();

}

void CArmManipulationControls::OnNew(wxCommandEvent& event)
{

  if (!ros::service::exists(SRV_NEW,true)) {

    m_text_status->SetLabel(wxString::FromAscii("status: communication error"));
    ROS_ERROR("Service %s is not ready.",((std::string)SRV_NEW).c_str());
    return;

  }

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  /// wait for some time
  if (t_new.timed_join(td)) {

    m_button_plan->Enable(false);
    m_button_reset->Enable(false);
    m_button_new->Enable(false);
    m_button_play->Enable(false);
    m_button_success->Enable(false);
    m_button_switch->Enable(false);
    m_button_repeat->Enable(false);

    m_text_status->SetLabel(wxString::FromAscii("status: Please wait..."));

    t_new = boost::thread(&CArmManipulationControls::NewThread,this);

  } else ROS_INFO("We have to wait until new thread finishes.");


}


void CArmManipulationControls::PlanThread() {

   ROS_INFO("Starting planning and filtering of new trajectory");

   std::string status = "";
   bool success = false;

   srs_assisted_arm_navigation::ArmNavPlan srv;

   if (ros::service::call(SRV_PLAN,srv) ) {

     success = true;

     if (srv.response.completed) status = "status: Trajectory is ready";
     else status = "status: " + srv.response.error;

   } else {

     success = false;
     ROS_ERROR("failed when calling service");
     status = "status: Communication error";

   }

   wxMutexGuiEnter();

   m_text_status->SetLabel(wxString::FromAscii(status.c_str()));

   if (success) {

     m_button_new->Enable(false);
     m_button_plan->Enable(false);
     m_button_play->Enable(true);
     m_button_execute->Enable(true);
     m_button_reset->Enable(true);
     m_button_success->Enable(false);
     m_button_repeat->Enable(true);
     m_button_failed->Enable(true);

   } else {

     m_button_new->Enable(false);
     m_button_plan->Enable(true);
     m_button_play->Enable(true);
     m_button_execute->Enable(true);
     m_button_reset->Enable(true);
     m_button_success->Enable(false);
     m_button_repeat->Enable(true);
     m_button_failed->Enable(true);


   }

   wxMutexGuiLeave();

}

void CArmManipulationControls::OnPlan(wxCommandEvent& event)
{

  if (!ros::service::exists(SRV_PLAN,true)) {

    m_text_status->SetLabel(wxString::FromAscii("status: communication error"));
    ROS_ERROR("Service %s is not ready.",((std::string)SRV_NEW).c_str());
    return;

  }

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  /// wait for some time
  if (t_plan.timed_join(td)) {

    m_text_status->SetLabel(wxString::FromAscii("status: Planning. Please wait..."));
    m_button_plan->Enable(false);
    m_button_reset->Enable(false);

    t_plan = boost::thread(&CArmManipulationControls::PlanThread,this);

  } else ROS_INFO("We have to wait until PLAN thread finishes.");

}

void CArmManipulationControls::OnPlay(wxCommandEvent& event)
{

   ROS_INFO("Starting planning and filtering of new trajectory");

   srs_assisted_arm_navigation::ArmNavPlay srv;

   if ( ros::service::exists(SRV_PLAY,true) && ros::service::call(SRV_PLAY,srv) ) {

     if (srv.response.completed) {

       m_text_status->SetLabel(wxString::FromAscii("status: Playing trajectory..."));

     } else {

       m_text_status->SetLabel(wxString::FromAscii(srv.response.error.c_str()));

     }

   } else {

     ROS_ERROR("failed when calling arm_nav_play service");
     m_text_status->SetLabel(wxString::FromAscii("status: Communication error"));

   }

   m_button_new->Enable(false);
   m_button_plan->Enable(false);
   m_button_play->Enable(false);
   m_button_execute->Enable(true);
   m_button_reset->Enable(true);
   m_button_success->Enable(false);
   m_button_repeat->Enable(true);
   m_button_failed->Enable(true);

}

void CArmManipulationControls::OnSwitch(wxCommandEvent& event)
{

  if (aco_) ROS_INFO("Lets switch attached collision object OFF");
  else ROS_INFO("Lets switch attached collision object ON");

   srs_assisted_arm_navigation::ArmNavSwitchAttCO srv;

   srv.request.state = !aco_;

   if ( ros::service::exists(SRV_SWITCH,true) && ros::service::call(SRV_SWITCH,srv) ) {

     if (srv.response.completed) {

       //m_text_status->SetLabel(wxString::FromAscii("status: Playing trajectory..."));

       aco_ = !aco_;

       if (aco_) m_button_switch->SetLabel(wxT("ACO enabled"));
       else m_button_switch->SetLabel(wxT("ACO disabled"));

     } else {

       m_text_status->SetLabel(wxString::FromAscii("Can't switch state of ACO"));

     }

   } else {

     std::string tmp = SRV_SWITCH;

     ROS_ERROR("failed when calling %s service",tmp.c_str());
     m_text_status->SetLabel(wxString::FromAscii("status: Communication error"));

   }


}


void CArmManipulationControls::OnExecute(wxCommandEvent& event) {

  if (!ros::service::exists(SRV_EXECUTE,true)) {

    m_text_status->SetLabel(wxString::FromAscii("status: communication error"));
    ROS_ERROR("Service %s is not ready.",((std::string)SRV_NEW).c_str());
    return;

  }

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  /// wait for some time
  if (t_execute.timed_join(td)) {

    t_execute = boost::thread(&CArmManipulationControls::ExecuteThread,this);

  } else ROS_INFO("We have to wait until EXECUTE thread finishes.");

}

void CArmManipulationControls::ExecuteThread()
{
   ROS_INFO("Execution of planned trajectory has been started");

   srs_assisted_arm_navigation::ArmNavExecute srv;

   std::string status = "";
   bool success = false;

   if (ros::service::call(SRV_EXECUTE,srv) ) {

     if (srv.response.completed) {

       success = true;
       status = "status: Executing trajectory...";

     } else {

       success = false;
       status = srv.response.error;

     }

   } else {

     ROS_ERROR("failed when calling arm_nav_execute service");
     success = false;
     status = "status: Communication error";

   }

   wxMutexGuiEnter();

   m_text_status->SetLabel(wxString::FromAscii(status.c_str()));

   if (success) {

     m_button_plan->Enable(false);
     m_button_execute->Enable(false);
     m_button_reset->Enable(false);
     m_button_play->Enable(false);
     m_button_new->Enable(true);
     m_button_success->Enable(true);
     m_button_failed->Enable(true);
     m_button_repeat->Enable(true);
     m_button_switch->Enable(true);

   } else {

     m_button_plan->Enable(false);
     m_button_execute->Enable(false);
     m_button_reset->Enable(true);
     m_button_play->Enable(false);
     m_button_new->Enable(false);
     m_button_success->Enable(false);
     m_button_repeat->Enable(true);
     m_button_failed->Enable(true);
     m_button_switch->Enable(true);

   }

   wxMutexGuiLeave();

}

void CArmManipulationControls::OnReset(wxCommandEvent& event)
{
   ROS_INFO("Reset planning stuff to initial state");

   srs_assisted_arm_navigation::ArmNavReset srv;

   if ( ros::service::exists(SRV_RESET,true) && ros::service::call(SRV_RESET,srv) ) {

     if (srv.response.completed) {

       m_text_status->SetLabel(wxString::FromAscii("status: Ok, try it again"));

     } else {

       m_text_status->SetLabel(wxString::FromAscii(srv.response.error.c_str()));

     }

   } else {

     ROS_ERROR("failed when calling arm_nav_reset service");
     m_text_status->SetLabel(wxString::FromAscii("status: Communication error"));

   }

   m_button_plan->Enable(false);
   m_button_execute->Enable(false);
   m_button_reset->Enable(false);
   m_button_play->Enable(false);
   m_button_new->Enable(true);
   m_button_success->Enable(false);
   m_button_repeat->Enable(true);
   m_button_failed->Enable(true);
   m_button_switch->Enable(true);

}

void CArmManipulationControls::OnSuccess(wxCommandEvent& event)
{
   ROS_INFO("Finishing manual arm manipulation task");

   srs_assisted_arm_navigation::ArmNavSuccess srv;

   if ( ros::service::exists(SRV_SUCCESS,true) && ros::service::call(SRV_SUCCESS,srv) ) {

       m_text_status->SetLabel(wxString::FromAscii("status: Succeeded :-)"));

   } else {

     ROS_ERROR("failed when calling arm_nav_success service");
     m_text_status->SetLabel(wxString::FromAscii("Communication error"));

   }

   m_button_plan->Enable(false);
   m_button_execute->Enable(false);
   m_button_reset->Enable(false);
   m_button_play->Enable(false);
   if (wait_for_start_)  m_button_new->Enable(false);
   else  m_button_new->Enable(true);

   m_text_object->SetLabel(wxString::FromAscii("object: none"));

   m_button_success->Enable(false);
   m_button_failed->Enable(false);
   m_button_repeat->Enable(false);
   m_button_switch->Enable(true);

   goal_pregrasp = false;
   goal_away = false;

}

void CArmManipulationControls::OnFailed(wxCommandEvent& event)
{
   ROS_ERROR("Manual arm manipulation task failed");

   srs_assisted_arm_navigation::ArmNavFailed srv;

   if ( ros::service::exists(SRV_FAILED,true) && ros::service::call(SRV_FAILED,srv) ) {

       m_button_failed->Enable(false);

       m_text_status->SetLabel(wxString::FromAscii("status: Failed :-("));
       m_text_object->SetLabel(wxString::FromAscii("object: none"));

   } else {

     ROS_ERROR("failed when calling arm_nav_failed service");
     m_text_status->SetLabel(wxString::FromAscii("Communication error"));

     m_button_failed->Enable(true);

   }

   m_button_plan->Enable(false);
   m_button_execute->Enable(false);
   m_button_reset->Enable(false);
   m_button_play->Enable(false);
   if (wait_for_start_) m_button_new->Enable(false);
   else m_button_new->Enable(true);

   m_button_success->Enable(false);
   m_button_repeat->Enable(false);
   m_button_repeat->Enable(false);
   m_button_switch->Enable(true);

   goal_pregrasp = false;
   goal_away = false;

}

void CArmManipulationControls::OnRepeat(wxCommandEvent& event)
{
   ROS_ERROR("Request for repeat of manual arm navigation task");

   srs_assisted_arm_navigation::ArmNavRepeat srv;

   if ( ros::service::exists(SRV_REPEAT,true) && ros::service::call(SRV_REPEAT,srv) ) {

       m_button_repeat->Enable(false);

       m_text_status->SetLabel(wxString::FromAscii("status: Repeating action..."));
       m_text_object->SetLabel(wxString::FromAscii("object: none"));

   } else {

     ROS_ERROR("failed when calling arm_nav_repeat service");
     m_text_status->SetLabel(wxString::FromAscii("Communication error"));

     m_button_repeat->Enable(true);

   }

   m_button_plan->Enable(false);
   m_button_execute->Enable(false);
   m_button_reset->Enable(false);
   m_button_play->Enable(false);
   if (wait_for_start_) m_button_new->Enable(false);
   else m_button_new->Enable(true);

   m_button_failed->Enable(false);
   m_button_success->Enable(false);
   m_button_repeat->Enable(false);
   m_button_switch->Enable(true);

   goal_pregrasp = false;
   goal_away = false;


}

bool CArmManipulationControls::InitCobScript() {

  if (cob_script==NULL) {

    return false;

      }

  if (!cob_script_inited) {

    if (!cob_script->waitForServer(ros::Duration(3.0))) {

        ROS_ERROR("No response from cob_script action server");
        return false;

      } else {

        cob_script_server::ScriptAction goal;

        goal.action_goal.goal.component_name = "sdh";
        goal.action_goal.goal.function_name = "init";
        goal.action_goal.goal.mode = "";
        goal.action_goal.goal.parameter_name = "";

        cob_script->sendGoal(goal.action_goal.goal);
        cob_script->waitForResult((ros::Duration(5.0)));

        if (cob_script->getState() != actionlib::SimpleClientGoalState::SUCCEEDED) return false;

        goal.action_goal.goal.component_name = "torso";

        cob_script->sendGoal(goal.action_goal.goal);
        cob_script->waitForResult((ros::Duration(5.0)));

        if (cob_script->getState() != actionlib::SimpleClientGoalState::SUCCEEDED) return false;

        cob_script_inited = true;

      }

  }

  if (cob_script_inited) return true;
  else return false;

}

/// @todo Add GUI lock mutex!!!!!!!
void CArmManipulationControls::GripperThread(unsigned char action) {

  if (!InitCobScript()) {

      /// @todo manage this situation somehow
        return;

    }

    std::string status = "";


    cob_script_server::ScriptAction goal;

    goal.action_goal.goal.component_name = "sdh";
    goal.action_goal.goal.function_name = "move";
    goal.action_goal.goal.mode = "";

    if (action==G_OPEN) goal.action_goal.goal.parameter_name = "cylopen";
    else goal.action_goal.goal.parameter_name = "cylclosed";

    cob_script->sendGoal(goal.action_goal.goal);

    cob_script->waitForResult((ros::Duration(5.0)));

    if (cob_script->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {


      if (action==G_OPEN) {

          //m_text_status->SetLabel(wxString::FromAscii("status: Gripper opened."));
          status = "status: Gripper opened.";
          ROS_INFO("Gripper should be opened...");
      }
      else {

        //m_text_status->SetLabel(wxString::FromAscii("status: Gripper closed."));
        status = "status: Gripper closed.";
        ROS_INFO("Gripper should be closed...");

      }

    } else {

      //m_text_status->SetLabel(wxString::FromAscii("status: Error during gripper action."));
      status = "status: Error during gripper action.";
      ROS_ERROR("Error on opening/closing gripper.");

    }

    wxMutexGuiEnter();

    m_text_status->SetLabel(wxString::FromAscii(status.c_str()));
    m_button_gripper_o->Enable(true);
    m_button_gripper_c->Enable(true);
    m_button_look_around->Enable(true);
    m_button_refresh->Enable(true);

    wxMutexGuiLeave();



}

void CArmManipulationControls::OnGripperO(wxCommandEvent& event) {

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  // wait for some time
  if (t_gripper.timed_join(td)) {

    unsigned char action = G_OPEN;

    m_button_gripper_o->Enable(false);
    m_button_gripper_c->Enable(false);
    m_button_look_around->Enable(false);
    m_button_refresh->Enable(false);

    m_text_status->SetLabel(wxString::FromAscii("status: Opening gripper."));

    t_gripper = boost::thread(&CArmManipulationControls::GripperThread,this,action);

  } else ROS_INFO("We have to wait until gripper thread finishes.");

}

/// @todo Add guimutex!!!!!!!!!!
void CArmManipulationControls::LookThread() {

  if (!InitCobScript()) {

      /// @todo manage it
      return;

  }

  std::string status = "";

  cob_script_server::ScriptAction goal;

  goal.action_goal.goal.component_name = "torso";
  goal.action_goal.goal.function_name = "move";
  goal.action_goal.goal.mode = "";
  goal.action_goal.goal.parameter_name = "front_left";

  /// @todo Are the timeouts sufficient?
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "front";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "front_right";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "back_right";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "back";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "back_left";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  goal.action_goal.goal.parameter_name = "home";
  cob_script->sendGoal(goal.action_goal.goal);
  cob_script->waitForResult((ros::Duration(5.0)));

  if (cob_script->getState() == actionlib::SimpleClientGoalState::SUCCEEDED) {

    //m_text_status->SetLabel(wxString::FromAscii("status: Looking around completed."));
    status = "status: Looking around completed.";
    ROS_INFO("Looking around completed - collision map should be improved.");

  } else {

      //m_text_status->SetLabel(wxString::FromAscii("status: Looking around failed."));
      status = "status: Looking around failed.";
      ROS_INFO("Looking around failed.");

  }


  wxMutexGuiEnter();

  m_text_status->SetLabel(wxString::FromAscii(status.c_str()));

  m_button_look_around->Enable(true);
  m_button_gripper_o->Enable(true);
  m_button_gripper_c->Enable(true);
  m_button_refresh->Enable(true);

  wxMutexGuiLeave();

}

void CArmManipulationControls::OnLook(wxCommandEvent& event) {

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  // wait for some time
  if (t_look.timed_join(td)) {

    m_button_gripper_o->Enable(false);
    m_button_gripper_c->Enable(false);
    m_button_look_around->Enable(false);
    m_button_refresh->Enable(false);

    m_text_status->SetLabel(wxString::FromAscii("status: Looking around."));
    ROS_INFO("Looking around to improve collision map.");

    t_look = boost::thread(&CArmManipulationControls::LookThread,this);

  } else ROS_INFO("We have to wait until LOOKAROUND thread finishes.");

}

/// @todo Add another thread - sometimes it takes a lot of time.
void CArmManipulationControls::OnRefresh(wxCommandEvent& event) {

  ROS_INFO("Refreshing planning scene");

   srs_assisted_arm_navigation::ArmNavRefresh srv;

   if ( ros::service::exists(SRV_REFRESH,true) && ros::service::call(SRV_REFRESH,srv) ) {

     if (srv.response.completed) {

       m_text_status->SetLabel(wxString::FromAscii("status: Refreshed."));

     } else {

       m_text_status->SetLabel(wxString::FromAscii("status: Error on refreshing."));

     }

   } else {

     ROS_ERROR("failed when calling arm_nav_refresh service");
     m_text_status->SetLabel(wxString::FromAscii("status: Communication error"));

   }

}

void CArmManipulationControls::OnGripperC(wxCommandEvent& event) {

  boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);

  // wait for some time
  if (t_gripper.timed_join(td)) {

    unsigned char action = G_CLOSE;

    m_button_gripper_o->Enable(false);
    m_button_gripper_c->Enable(false);
    m_button_look_around->Enable(false);
    m_button_refresh->Enable(false);

    m_text_status->SetLabel(wxString::FromAscii("status: Closing gripper."));

    t_gripper = boost::thread(&CArmManipulationControls::GripperThread,this,action);

  } else ROS_INFO("We have to wait until gripper thread finishes.");

}

bool CArmManipulationControls::nav_start(srs_assisted_arm_navigation::ArmNavStart::Request &req, srs_assisted_arm_navigation::ArmNavStart::Response &res) {


  if (req.pregrasp) {

    char str[80];

    snprintf(str,80,"Please navigate arm to pregrasp position for: %s",req.object_name.c_str());

    wxMessageBox(wxString::FromAscii(str), wxString::FromAscii("Manual arm navigation"), wxOK, parent_,-1,-1);


    goal_pregrasp = true;
    goal_away = false;
    object_name = req.object_name;

    std::string tmp;
    tmp = std::string("object_name: ") + object_name;
    m_text_object->SetLabel(wxString::FromAscii(tmp.c_str()));

  }

  if (req.away) {

      wxMessageBox(wxString::FromAscii("Please navigate arm to safe position"), wxString::FromAscii("Manual arm navigation"), wxOK, parent_,-1,-1);

      goal_pregrasp = false;
      goal_away = true;

    }

  m_button_new->Enable(true);
  res.completed = true;

  return true;

};



///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(CArmManipulationControls, wxPanel)
    EVT_BUTTON(ID_BUTTON_NEW,  CArmManipulationControls::OnNew)
    EVT_BUTTON(ID_BUTTON_PLAN,  CArmManipulationControls::OnPlan)
    EVT_BUTTON(ID_BUTTON_PLAY,  CArmManipulationControls::OnPlay)
    EVT_BUTTON(ID_BUTTON_EXECUTE,  CArmManipulationControls::OnExecute)
    EVT_BUTTON(ID_BUTTON_RESET,  CArmManipulationControls::OnReset)
    EVT_BUTTON(ID_BUTTON_SUCCESS,  CArmManipulationControls::OnSuccess)
    EVT_BUTTON(ID_BUTTON_FAILED,  CArmManipulationControls::OnFailed)
    EVT_BUTTON(ID_BUTTON_GRIPPER_O,  CArmManipulationControls::OnGripperO)
    EVT_BUTTON(ID_BUTTON_GRIPPER_C,  CArmManipulationControls::OnGripperC)
    EVT_BUTTON(ID_BUTTON_LOOK,  CArmManipulationControls::OnLook)
    EVT_BUTTON(ID_BUTTON_REFRESH,  CArmManipulationControls::OnRefresh)
    EVT_BUTTON(ID_BUTTON_SWITCH,  CArmManipulationControls::OnSwitch)
    EVT_BUTTON(ID_BUTTON_REPEAT,  CArmManipulationControls::OnRepeat)
END_EVENT_TABLE()
