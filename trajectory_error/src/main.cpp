#include<iostream>
#include<fstream>
#include<math.h>
#include<std_msgs/String.h>
#include<nav_msgs/Path.h>
#include<ros/ros.h>
#include<tf/transform_broadcaster.h>
#include<tf/transform_listener.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/PoseStamped.h>
//#include<Eigen/core>
//#include<Eigen/Geometry>
#include<eigen3/Eigen/Core>
#include<eigen3/Eigen/Geometry>
#include<sophus/se3.h>

std::string groundtruth_file;
std::string estimated_file;

template <class Type>
Type stringToNum(const std::string& str)
{
    std::istringstream iss(str);
    Type num;
    iss >> num;

    return num;
}

class trajectory_error
{
    public:
        trajectory_error();
        int read_file();
        double conputer_error(std::string& file1,std::string& file);
        void pub_msg(std::string& time,Eigen::Vector3d & t,Eigen::Quaterniond & q,nav_msgs::Path & path,ros::Publisher pub);
    private:

        double RMSE,err;
        int num;
        nav_msgs::Path path_groundtruth,path_estimated;
        ros::Publisher groundtruth_path_pub,estimated_path_pub;
        ros::NodeHandle node_;
};
trajectory_error::trajectory_error()
{
    err = 0;num=0;
    groundtruth_path_pub = node_.advertise<nav_msgs::Path>("groundtruth_path_pub",1, true);
    estimated_path_pub = node_.advertise<nav_msgs::Path>("estimated_path_pub",1, true); 
    path_estimated.header.stamp=ros::Time::now();
    path_estimated.header.frame_id="odom";
    path_groundtruth.header.stamp=ros::Time::now();
    path_groundtruth.header.frame_id="odom";
}

double trajectory_error::conputer_error(std::string& file_groundtruth,std::string& file_estimated)
{
    std::ifstream fin_groundtruth(file_groundtruth.c_str());  //why c_str() ?  地址 ？
    std::ifstream fin_estimated(file_estimated.c_str());
    if(fin_groundtruth.is_open() == false)
    {
        std::cout<<"Read file "<<file_groundtruth<<" Failed"<<std::endl;
        return 0;
    }
    if(fin_estimated.is_open() == false)
    {
        std::cout<<"Read file "<<file_estimated<<" Failed"<<std::endl;
        return 0;
    }
    std::string line_groundtruth,line_estimated;
    while(std::getline(fin_groundtruth, line_groundtruth) 
          && std::getline(fin_estimated, line_estimated))
    {
        Sophus::SE3 T_W_Cg, T_W_Ce;
        Eigen::Vector3d t_g, t_e;
        Eigen::Quaterniond Q_g,Q_e; 
        Eigen::Matrix<double,6,1> kesi;
        std::string time_g,time_e;
        std::stringstream stream_groundtruth(line_groundtruth), stream_estimated(line_estimated);
        stream_groundtruth>>time_g>>t_g[0]>>t_g[1]>>t_g[2]>>Q_g.x()>>Q_g.y()>>Q_g.z()>>Q_g.w();
        stream_estimated>>time_e>>t_e[0]>>t_e[1]>>t_e[2]>>Q_e.x()>>Q_e.y()>>Q_e.z()>>Q_e.w();
        T_W_Cg = Sophus::SE3(Q_g,t_g);
        T_W_Ce = Sophus::SE3(Q_e,t_e);
        kesi = (T_W_Cg.inverse()*T_W_Ce).log();
        err += kesi.transpose() * kesi;
        num++;
         
        pub_msg(time_g,t_g,Q_g,path_groundtruth,groundtruth_path_pub);
        pub_msg(time_e,t_e,Q_e,path_estimated,estimated_path_pub);
        
    }
    if(!std::getline(fin_groundtruth, line_groundtruth) 
          || !std::getline(fin_estimated, line_estimated))
    {
        RMSE = sqrt(err/num);
    }
    return RMSE;
    
}

void trajectory_error::pub_msg(std::string& time,Eigen::Vector3d & t,Eigen::Quaterniond & q,nav_msgs::Path & path,ros::Publisher pub)
{
    std::stringstream stream(time);
    std::string sec , nsec; 
    std::getline(stream,sec,',');
    std::getline(stream,nsec,',');
    geometry_msgs::PoseStamped this_pose_stamped;
    this_pose_stamped.header.stamp.sec=stringToNum<int>(sec);
    this_pose_stamped.header.stamp.nsec=stringToNum<int>(nsec);
    this_pose_stamped.header.frame_id="odom";

    this_pose_stamped.pose.position.x = t[0];
    this_pose_stamped.pose.position.y = t[1];
    this_pose_stamped.pose.position.y = t[2];

    this_pose_stamped.pose.orientation.x = q.x();
    this_pose_stamped.pose.orientation.y = q.y();
    this_pose_stamped.pose.orientation.z = q.z();
    this_pose_stamped.pose.orientation.w = q.w();

    path.poses.push_back(this_pose_stamped);
    
    pub.publish(path);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "trajectory_error");
    ros::NodeHandle n("~");   //NOTE 获取私有参数要加 “～”
    if(!private_n.getParam("groundtruth_file",groundtruth_file))
        std::cout<<"NO groundtruth_file"<<std::endl;
    private_n.getParam("estimated_file",estimated_file);
    std::cout<<argc<<"estimated_file"<<estimated_file<<std::endl;
    trajectory_error te;

    std::cout<<"RMSE : "<<te.conputer_error(groundtruth_file,estimated_file)<<std::endl;;
    ros::spin();
    return 0;
}



