#ifndef CHEETAH_SOFTWARE_CONVEXMPCLOCOMOTION_H
#define CHEETAH_SOFTWARE_CONVEXMPCLOCOMOTION_H
/*MIT-Cheetah Software
**             Email:@qq.com   QQ:1370780559
**---------------------------------------------------------
**  Description: 此文件注释由本人完成，仅为个人理解,本人水平有限，还请见谅
**  interpreter     : NaCl
*/
#include <Controllers/FootSwingTrajectory.h>
#include <FSM_States/ControlFSMData.h>
#include <SparseCMPC/SparseCMPC.h>
#include "cppTypes.h"
#include "Gait.h"

#include <cstdio>

using Eigen::Array4f;
using Eigen::Array4i;


template<typename T>
struct CMPC_Result {
  LegControllerCommand<T> commands[4];
  Vec4<T> contactPhase;
};

struct CMPC_Jump {
  static constexpr int START_SEG = 6;
  static constexpr int END_SEG = 0;
  static constexpr int END_COUNT = 2;
  bool jump_pending = false;
  bool jump_in_progress = false;
  bool pressed = false;
  int seen_end_count = 0;
  int last_seg_seen = 0;
  int jump_wait_counter = 0;

  void debug(int seg) {
    (void)seg;
    //printf("[%d] pending %d running %d\n", seg, jump_pending, jump_in_progress);
  }

  void trigger_pressed(int seg, bool trigger) {
    (void)seg;
    if(!pressed && trigger) {
      if(!jump_pending && !jump_in_progress) {
        jump_pending = true;
        //printf("jump pending @ %d\n", seg);
      }
    }
    pressed = trigger;
  }

  bool should_jump(int seg) {
    debug(seg);

    if(jump_pending && seg == START_SEG) {
      jump_pending = false;
      jump_in_progress = true;
      //printf("jump begin @ %d\n", seg);
      seen_end_count = 0;
      last_seg_seen = seg;
      return true;
    }

    if(jump_in_progress) {
      if(seg == END_SEG && seg != last_seg_seen) {
        seen_end_count++;
        if(seen_end_count == END_COUNT) {
          seen_end_count = 0;
          jump_in_progress = false;
          //printf("jump end @ %d\n", seg);
          last_seg_seen = seg;
          return false;
        }
      }
      last_seg_seen = seg;
      return true;
    }

    last_seg_seen = seg;
    return false;
  }
};


class ConvexMPCLocomotion {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  ConvexMPCLocomotion(float _dt, int _iterations_between_mpc, MIT_UserParameters* parameters);
  void initialize();

  template<typename T>
  void run(ControlFSMData<T>& data);
  bool currently_jumping = false;

  Vec3<float> pBody_des;//期望 世界下？//将相关数据统统暴露出 可供外部读取
  Vec3<float> vBody_des;//将相关数据统统暴露出 可供外部读取
  Vec3<float> aBody_des;//将相关数据统统暴露出 可供外部读取

  Vec3<float> pBody_RPY_des;//将相关数据统统暴露出 可供外部读取
  Vec3<float> vBody_Ori_des;//将相关数据统统暴露出 可供外部读取

  Vec3<float> pFoot_des[4];//足端期望 足端轨迹跟踪用 
  Vec3<float> vFoot_des[4];//将相关数据统统暴露出 可供外部读取
  Vec3<float> aFoot_des[4];//将相关数据统统暴露出 可供外部读取

  Vec3<float> Fr_des[4];//将相关数据统统暴露出 可供外部读取

  Vec4<float> contact_state;

private:
  void _SetupCommand(ControlFSMData<float> & data);
////期望 机体下
  float _yaw_turn_rate;
  float _yaw_des;

  float _roll_des;
  float _pitch_des;

  float _x_vel_des = 0.;
  float _y_vel_des = 0.;

  // High speed running
  //float _body_height = 0.34;
  float _body_height = 0.29;//机身高度

  float _body_height_running = 0.29;
  float _body_height_jumping = 0.36;

  void recompute_timing(int iterations_per_mpc);
  void updateMPCIfNeeded(int* mpcTable, ControlFSMData<float>& data, bool omniMode);//更新MPC数据
  void solveDenseMPC(int *mpcTable, ControlFSMData<float> &data);//解MPC
  void solveSparseMPC(int *mpcTable, ControlFSMData<float> &data);//解稀疏MPC
  void initSparseMPC();//初始化稀疏MPC
  int iterationsBetweenMPC;//迭代次数在mpc之间
  int horizonLength;//mpc分段数，即预测多少个mpc周期长的输入
  int default_iterations_between_mpc;//默认迭代次数在mpc之间
  float dt;//一般频率下时间间隔
  float dtMPC;//mpc运算周期
  int iterationCounter = 0;//频率控制计数器
  Vec3<float> f_ff[4];//四脚力输出
  Vec4<float> swingTimes;//摆动时间 
  FootSwingTrajectory<float> footSwingTrajectories[4];
  OffsetDurationGait trotting, bounding, pronking, jumping, galloping, standing, trotRunning, walking, walking2, pacing;
  MixedFrequncyGait random, random2;
  Mat3<float> Kp, Kd, Kp_stance, Kd_stance;
  bool firstRun = true;//首次运行
  bool firstSwing[4];//首次摆动
  
  float swingTimeRemaining[4];//剩余摆动时间
  
  float stand_traj[6];
  int current_gait;//当前步态
  int gaitNumber;//步态

  Vec3<float> world_position_desired;//机体期望位置
  Vec3<float> rpy_int;//初始欧拉角
  Vec3<float> rpy_comp;
  float x_comp_integral = 0;
  Vec3<float> pFoot[4];//四足端坐标
  CMPC_Result<float> result;
  float trajAll[12*36];//mpc格式储存轨迹

  MIT_UserParameters* _parameters = nullptr;
  CMPC_Jump jump_state;

  vectorAligned<Vec12<double>> _sparseTrajectory;

  SparseCMPC _sparseCMPC;

};


#endif //CHEETAH_SOFTWARE_CONVEXMPCLOCOMOTION_H
