#!/usr/bin/python
#################################################################
##\file
#
# \note
# Copyright (c) 2011 \n
# Cardiff University \n\n
#
#################################################################
#
# \note
# Project name: Multi-Role Shadow Robotic System for Independent Living
# \note
# ROS stack name: srs
# \note
# ROS package name: srs_decision_making
#
# \author
# Author: Renxi Qiu, email: renxi.qiu@gmail.com
#
# \date Date of creation: Oct 2011
#
# \brief
# Task coordination and interfacing for SRS decision making
#
#################################################################
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer. \n
#
# - Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution. \n
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License LGPL as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License LGPL for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License LGPL along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#
#################################################################

import roslib; roslib.load_manifest('srs_decision_making')

#roslib.load_manifest('knowledge_ros_service')

import rospy
import smach
import smach_ros
import sys, os, traceback, optparse, time, string, threading
from actionlib import *
from actionlib.msg import *
from smach import Iterator, StateMachine, CBState
from smach_ros import ConditionState, IntrospectionServer

from srs_knowledge.srv import *
from srs_knowledge.msg import *

from cob_tray_sensors.srv import *

"""
smach introspection server not working in electric yet, modify the executive_smach/smach_msgs/msg/SmachContainerStatus.msg below can bypass error:

# A pickled user data structure
string local_data
to:
# A pickled user data structure
uint8[] local_data

and

--- a/smach_ros/src/smach_ros/introspection.py  Tue Aug 02 10:31:19 2011 -0700
+++ b/smach_ros/src/smach_ros/introspection.py  Thu Sep 29 22:31:48 2011 +0900
@@ -219,7 +219,7 @@
                     path,
                     self._container.get_initial_states(),
                     self._container.get_active_states(),
-                    pickle.dumps(self._container.userdata._data,2),
+                    "",
                     info_str)
             # Publish message
             self._status_pub.publish(state_msg)
"""
from srs_configuration_statemachines import *
            
"""
                sub-statemachines in use:
                ###########################################
                sm_approach_pose_assisted()
                #assisted navigation, operator could specify intermediate position for final goal
               
                sm_detect_asisted_pose()
                #operator or KB could adjust the scanning pose 
                
                sm_open_door()
                #open door from IPA example
              
                sm_pick_object_basic()
                #basic routine for pick object from IPA example

                sm_detect_asisted_region()
                #operator or KB could specify region of interest
                #require generic state detect_object()taking key_region as input_key
                
                sm_detect_asisted_pose_region()
                #detect object, both base pose and region can be adjusted 
                #require generic state detect_object()taking key_region as input_key
                
                sm_pick_object_asisted()
                #pick object with user intervention for error handling
                #require generic state grasp_general()taking grasp_conf as input_key
                
                sm_enviroment_object_update()
                #updating the environment and find object
                #require generic state verify_object() and update_env_model()
                
                ##########################################            
                
                states need to be developed within SRS
                ##########################################
                detect_object()
                #taking key_region as input_key
                #from srs_asisted_detect module
                
                grasp_general()
                #taking grasp_conf as input_key
                #from srs_asisted_grasp module
                
                verify_object() 
                # model fitting from PRO

                
                update_env_model()
                # update environment model from IPA
                ##########################################                     
"""



#Customised Action sever designed for overwriting SimpleActionServer pre-empty sequence
class SRSActionServer(SimpleActionServer):
    def _init_(self, name, ActionSpec, execute_cb, auto_start):
        super(SRSActionServer, self).__init__(name, ActionSpec, execute_cb, auto_start)       

    def take_next_goal(self):
        #accept the next goal
        rospy.loginfo("Receiving a new goal")
        self.current_goal = self.next_goal
        self.new_goal = False
        #set preempt to request to equal the preempt state of the new goal
        rospy.loginfo("preempt_request %s", self.preempt_request)
        rospy.loginfo("new_goal preempt_request %s", self.new_goal_preempt_request)
        self.preempt_request = self.new_goal_preempt_request
        self.new_goal_preempt_request = False
        #set the status of the current goal to be active
        self.current_goal.set_accepted("This goal has been accepted by the simple action server");
        return self.current_goal.get_goal()

    
    def accept_new_goal(self):
        with self.lock:
            if not self.new_goal or not self.next_goal.get_goal():
                rospy.loginfo("Attempting to accept the next goal when a new goal is not available");
                return None;
 
             #check if we need to send a preempted message for the goal that we're currently pursuing
            if self.is_active() and self.current_goal.get_goal() and self.current_goal != self.next_goal:
                 #self.current_goal.set_canceled(None, "This goal was cancelled because another goal was received by the simple action server");
                 self.next_goal.set_rejected(None, "This goal was rejected because the server is busy with another task")
                 rospy.loginfo("Postpone a new goal")
            else:
                #accept the next goal
                rospy.loginfo("Receiving a new goal")
                self.current_goal = self.next_goal
                self.new_goal = False
                #set preempt to request to equal the preempt state of the new goal
                rospy.loginfo("preempt_request %s", self.preempt_request)
                rospy.loginfo("new_goal preempt_request %s", self.new_goal_preempt_request)
                
                self.preempt_request = self.new_goal_preempt_request
                self.new_goal_preempt_request = False
                #set the status of the current goal to be active
                self.current_goal.set_accepted("This goal has been accepted by the simple action server");
                
 
            return self.current_goal.get_goal()
        
        
        """
        with self.lock:
            if not self.new_goal or not self.next_goal.get_goal():
                rospy.loginfo("Attempting to accept the next goal when a new goal is not available");
            
            global current_task_info
            
            _result = xmsg.ExecutionResult()
            _result.return_value=3
            
            
            #check if we need to send a preempted message for the goal that we're currently pursuing
            if self.is_active() and self.current_goal.get_goal() and self.current_goal != self.next_goal:
                #self.current_goal.set_canceled(None, "This goal was cancelled because another goal was received by the simple action server");
                self.next_goal.set_rejected(None, "This goal was rejected because the server is busy with another task")
                rospy.loginfo("Postpone a new goal")  
       
            elif self.next_goal.get_goal().action == 'pause':
                current_task_info.set_pause_required(True) 
                self.next_goal.set_accepted("")
                self.next_goal.set_succeeded(_result, 'pause acknowledged')

            elif self.next_goal.get_goal().action == 'resume':
                if current_task_info.get_pause_required() == True:   # already paused
                    current_task_info.set_pause_required(False)
                    self.next_goal.set_accepted("")
                    self.next_goal.set_succeeded(_result, 'resume acknowledged')
                else:
                    _result.return_value = 2
                    self.next_goal.set_aborted(_result, 'not paused, no need for resuming')
                    pass 

            else:
                if not self.current_goal.get_goal():
                    # current goal is null
                    return self.take_next_goal()
                if self.next_goal.get_goal().action == 'stop':
                    current_task_info.set_stop_required(True) 
                    return self.take_next_goal()
                if self.next_goal.get_goal().priority > self.current_goal.get_goal().priority:                             
                    current_task_info.set_customised_preempt_required(True)   
                    return self.take_next_goal()
            
            return None
        """

         
class SRS_DM_ACTION(object):
    #action server for taking tasks 
    def __init__(self, name):

        self._action_name = name
        self._as = SRSActionServer(self._action_name, xmsg.ExecutionAction, self.execute_cb, auto_start=False) 
        self._feedback = xmsg.ExecutionFeedback()
        self._result = xmsg.ExecutionResult()
        self._task = ""
        self._parameter = ""
        self._as.start()
        self.robot_initialised= False
         
                
        #self._as.register_goal_callback(self.goal_cb)
        self._as.register_preempt_callback(self.priority_cb)
        
        rospy.loginfo("Waiting for wake up the server ...")
        
    

        
    def robot_initialisation_process(self):
        if not self.robot_initialised :
            #initialisation of the robot
            # move to initial positions
            global sss
            #handle_torso = sss.move("torso", "home", False)
            #handle_tray = sss.move("tray", "down", False)
            #handle_arm = sss.move("arm", "folded", False)
            #handle_sdh = sss.move("sdh", "cylclosed", False)
            #handle_head = sss.move("head", "front", False)
    
        
            # wait for initial movements to finish
            #handle_torso.wait()
            #handle_tray.wait()
            #handle_arm.wait()
            #handle_sdh.wait()
            #handle_head.wait()
            self.robot_initialised = True
            
        

        
    def init_sm(self):
        self.temp = smach.StateMachine(outcomes=['task_succeeded','task_aborted', 'task_preempted'])
        
        #
        self.temp.userdata.target_base_pose=Pose2D()
        self.temp.userdata.target_object_name=''
        self.temp.userdata.target_object_pose=Pose()
        self.temp.userdata.the_target_object_found = 'sadsadsadsadsda'
        self.temp.userdata.verified_target_object_pose=Pose()
        
        #session id for current task, on id per task.
        #session id can be shared by different clients
        self.session_id = 0
        
        #user intervention is possible or not.
        #False, DM has to make all the decision by it self
        #True, suitable client has been connected, can be relied on
        self.temp.userdata.semi_autonomous_mode=True
        
        
     # open the container
        with self.temp:
            # add states to the container
            smach.StateMachine.add('INITIALISE', initialise(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'failed':'task_aborted'})
                        
            smach.StateMachine.add('SEMANTIC_DM', semantic_dm(),
                                   transitions={'succeeded':'task_succeeded',
                                                'failed':'task_aborted',
                                                'preempted':'task_preempted',
                                                'navigation':'SM_NAVIGATION',
                                                'detection':'SM_DETECTION',
                                                'simple_grasp':'SM_OLD_GRASP',
                                                'full_grasp':'SM_NEW_GRASP',
                                                'put_on_tray':'SM_PUT_ON_TRAY',
                                                'env_update':'SM_ENV_UPDATE'},
                                   remapping={'target_base_pose':'target_base_pose',
                                               'target_object_name':'target_object_name',
                                               'target_object_pose':'target_object_pose',
                                               'target_object_id':'target_object_id',
                                               'target_object':'the_target_object_found',
                                               'target_workspace_name':'target_workspace_name',
                                               'semi_autonomous_mode':'semi_autonomous_mode',
                                               'grasp_categorisation':'grasp_categorisation',
                                               'scan_pose_list':'scan_pose_list'})
            
            smach.StateMachine.add('SM_NAVIGATION', srs_navigation_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'target_base_pose':'target_base_pose',
                                               'semi_autonomous_mode':'semi_autonomous_mode'})

            smach.StateMachine.add('SM_DETECTION', srs_detection_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'target_object_name':'target_object_name',
                                              'target_object_id':'target_object_id',
                                              'target_workspace_name':'target_workspace_name',
                                              'semi_autonomous_mode':'semi_autonomous_mode',
                                              'target_object':'the_target_object_found',
                                              'target_object_pose':'target_object_pose' })
       
            smach.StateMachine.add('SM_NEW_GRASP', srs_grasp_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'target_object_name':'target_object_name',
                                              'target_object_id':'target_object_id',
                                              'target_workspace_name':'target_workspace_name',
                                              'semi_autonomous_mode':'semi_autonomous_mode',
                                              'target_object':'the_target_object_found',
                                              'target_object_pose':'target_object_pose',
                                              'grasp_categorisation':'grasp_categorisation'})
            
            '''
            START
            #Old grasp added for backward compatible, should be removed after knowledge service updated completely
            '''
            smach.StateMachine.add('SM_OLD_GRASP', srs_old_grasp_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'target_object_name':'target_object_name',
                                              'semi_autonomous_mode':'semi_autonomous_mode',
                                              'target_object_id':'target_object_id',
                                              'target_object':'the_target_object_found',
                                              'grasp_categorisation':'grasp_categorisation'})
            '''
            #Old grasp added for backward compatible, should be removed after knowledge service updated completely
            END
            '''
            
            smach.StateMachine.add('SM_PUT_ON_TRAY', srs_put_on_tray_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'grasp_categorisation':'grasp_categorisation' })

            smach.StateMachine.add('SM_ENV_UPDATE', srs_enviroment_update_operation(),
                                   transitions={'succeeded':'SEMANTIC_DM', 'not_completed':'SEMANTIC_DM', 'failed':'SEMANTIC_DM','stopped':'task_preempted','preempted':'task_preempted'},
                                   remapping={'target_object_pose':'target_object_pose',
                                              'target_object_hh_id':'target_object_hh_id',
                                              'verified_target_object_pose':'verified_target_object_pose'})

                        

        return self.temp
                    
            
    
    def priority_cb(self):
        #rospy.loginfo("setting priority")
        # when this function is called self._as.preempt_request = True due to default policy of the simple action server
        # this function override the default policy by compare the priority 
        self._as.preempt_request = False # overwrite the default preempt policy of the simple action server
        
        global current_task_info
        result = xmsg.ExecutionResult()
        result.return_value=3
        #checking if there is a new goal and comparing the priority    
        if self._as.next_goal.get_goal() and self._as.new_goal:
            print self._as.next_goal.get_goal()
            print self._as.new_goal
            
            #new goal exist
            if self._as.next_goal.get_goal().action == 'stop':
                current_task_info.set_stop_required(True)
            elif self._as.next_goal.get_goal().action == 'pause':
                current_task_info.set_pause_required(True) 
                self._as.next_goal.set_accepted("")
                self._as.next_goal.set_succeeded(result, 'pause acknowledged')
                self._as.new_goal=False
                self._as.new_goal_preempt_request = False
            elif self._as.next_goal.get_goal().action == 'resume':
                if current_task_info.get_pause_required() == True:   # already paused
                    current_task_info.set_pause_required(False)
                    self._as.next_goal.set_accepted("")
                    self._as.next_goal.set_succeeded(result, 'resume acknowledged')
                else:
                    result.return_value=2
                    self._as.next_goal.set_accepted("")
                    self._as.next_goal.set_aborted(result, 'not paused, no need for resuming') 
                self._as.new_goal=False
                self._as.new_goal_preempt_request = False
            elif self._as.next_goal.get_goal().priority > self._as.current_goal.get_goal().priority:
                current_task_info.set_customised_preempt_required(True)
        else:
            # pre-empty request is come from the same goal 
            self._sm_srs.request_preempt()             
              
    #def preempt_check(self):
    #    if self.get_customised_preempt_required()==True:
    #        self.set_customised_preempt_required(False)
    #        return True
    #    return False
   
        
    def execute_cb(self, gh):

        self._feedback.current_state = "initialisation"
        self._feedback.solution_required = False
        self._feedback.exceptional_case_id = 0
        self._as.publish_feedback(self._feedback)
        #goal=self._as.current_goal.get_goal()
        rospy.Subscriber("fb_executing_solution", Bool, self.callback_fb_solution_req)
        rospy.Subscriber("fb_executing_state", String, self.callback_fb_current_state)
        
        goal_handler = self._as.current_goal
        current_goal = goal_handler.get_goal()      
        
        #initialise task information for the state machine
        global current_task_info
        current_task_info.task_name = current_goal.action
        if current_task_info.task_name=="":
            current_task_info.task_name="get"
        current_task_info.task_parameter = current_goal.parameter
        ## added by Ze
        current_task_info.task_parameters = current_goal.parameters
        
        if current_task_info.task_name=='stop':
            current_task_info.set_stop_acknowledged(False)            #
            current_task_info.set_stop_required(False)
        
        if not self.robot_initialised:
            self.robot_initialisation_process()
            
        current_task_info._srs_as = copy.copy(self)
        


        ##############################################
        # taskrequest From Knowledge_ros_service
        ##############################################
        print '#######################   Action Accept Command'
        print gh
        print 'Request new task'
        rospy.wait_for_service('task_request')
        try:
            requestNewTask = rospy.ServiceProxy('task_request', TaskRequest)
            #res = requestNewTask(current_task_info.task_name, current_task_info.task_parameter, None, None, None, None)
            req = TaskRequestRequest()
            req.task = current_task_info.task_name
            
            ## added by ze
            pars = current_task_info.task_parameter.split(",")    
            length = len(pars)
            if length == 1:
                req.content = current_task_info.task_parameter
                req.userPose = "order"
            elif length == 2:
                req.content = pars[0]
                req.userPose = pars[1]
            print req.content
            print req.userPose
            req.parameters = current_task_info.parameters
            
            res = requestNewTask(req)
            #res = requestNewTask(current_task_info.task_name, current_task_info.task_parameter, "order")
            print 'Task created with session id of: ' + str(res.sessionId)
            current_task_info.session_id = res.sessionId
            if res.result == 1:
                self._as.set_aborted(self._result)
                return
            #elif res.result == 0:
                
        except rospy.ServiceException, e:
            print "Service call failed: %s"%e
        ##############################################
        # END OF taskrequest From Knowledge_ros_service
        ##############################################


        
        #initial internal state machine far task 
        # As action server, the SRSActionServer is also a state machine with basic priority handling ability by itself,
        # Therefore there is no need for a 'idle'/'wait for task' state in this state machine.
        self._sm_srs = self.init_sm()
        self.sis=smach_ros.IntrospectionServer('TASK_SM',self._sm_srs,'/SM_ROOT')
        #display smach state
        self.sis.start()           
        self._as.is_active()
        #run the state machine for the task
        outcome = self._sm_srs.execute()
        self.sis.stop()
        
        #Testing recorded task execution history
        last_step=current_task_info.last_step_info.pop()
        rospy.loginfo("sm last step name: %s", last_step.step_name)
        rospy.loginfo("sm last step session ID: %s", current_task_info.session_id)
        
        #set outcomes based on the execution result       
                
        #if self.preempt_check()==True:
        #    self._result.return_value=2
        #    self._as.set_preempted(self._result)
        #    return
        
        
        current_task_info.set_customised_preempt_acknowledged(False)
        current_task_info.set_customised_preempt_required(False)
        current_task_info.set_stop_acknowledged(False)
        current_task_info.set_stop_required(False)        
        current_task_info.set_object_identification_state(False) 
        
        if outcome == "task_succeeded": 
            self._result.return_value=3
            self._as.set_succeeded(self._result)
            return
        if outcome == "task_preempted":
            self._result.return_value=2
            self._as.set_preempted(self._result, "stopped before complete or preempted by another task")
        #for all other cases outcome == "task_aborted": 
        self._result.return_value=4
        self._as.set_aborted(self._result)

            
    def callback_fb_solution_req(self, data):
        #rospy.loginfo("I heard %s",data.data)
        self._feedback.solution_required = data.data
        self._as.publish_feedback(self._feedback)
        
    def callback_fb_current_state(self, data):
        #rospy.loginfo("I heard %s",data.data)
        self._feedback.current_state = data.data
        self._as.publish_feedback(self._feedback)        
         
    def task_executor(self, gh):
        pass


if __name__ == '__main__':
    rospy.init_node('srs_decision_making_actions')
    SRS_DM_ACTION(rospy.get_name())
    rospy.spin()
    #global listener    
    #listener = tf.TransformListener()

