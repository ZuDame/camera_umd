/* JPEG CompressedImage -> HTTP streaming node for ROS
 * (c) 2010 Ken Tossell / ktossell@umd.edu
 */
#include <ros/ros.h>
#include <sensor_msgs/CompressedImage.h>
#include <image_transport/image_transport.h>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>
#include "mongoose.h"

using namespace std;

class JPEGStreamer {
  public:
    JPEGStreamer();
    void add_connection(struct mg_connection *con, boost::condition_variable *cond, boost::mutex *single_mutex);

  private:
    ros::NodeHandle node;
    ros::Subscriber image_sub;
    mg_context *web_context;
    boost::mutex con_mutex;
    boost::mutex data_mutex;
    list<boost::tuple<struct mg_connection*, boost::condition_variable*, boost::mutex*> > connections;

    void image_callback(const sensor_msgs::CompressedImage::ConstPtr& msg);
};

static JPEGStreamer *g_status_video;

int new_req_callback(struct mg_connection *con, const struct mg_request_info *req) {
  boost::condition_variable cond;
  boost::mutex single_mutex;
  boost::unique_lock<boost::mutex> lock(single_mutex);
  g_status_video->add_connection(con, &cond, &single_mutex);
  cond.wait(lock);
  return 1;
}

JPEGStreamer::JPEGStreamer() {
  string topic = node.resolveName("image");
  string port;
  node.param("port", port, string("8080"));

  image_sub = node.subscribe<sensor_msgs::CompressedImage>(topic, 1,
    boost::bind(&JPEGStreamer::image_callback, this, _1)
  );

  g_status_video = this;

  web_context = mg_start();
  mg_set_option(web_context, "ports", port.c_str());
  mg_set_callback(web_context, MG_EVENT_NEW_REQUEST, &new_req_callback);
}

static char header_text[] = "HTTP/1.0 200 OK\r\nConnection: Close\r\n"
  "Server: ros/jpeg_streamer\r\n"
  "Content-Type: multipart/x-mixed-replace;boundary=--myboundary\r\n\r\n";

void JPEGStreamer::add_connection(struct mg_connection *con, boost::condition_variable *cond, boost::mutex *single_mutex) {
  mg_write(con, header_text, sizeof(header_text));

  {
    boost::mutex::scoped_lock lock(con_mutex);

    connections.push_back(boost::tuple<struct mg_connection*, boost::condition_variable*, boost::mutex*>(con, cond, single_mutex));
  }
}

void JPEGStreamer::image_callback(const sensor_msgs::CompressedImage::ConstPtr& msg) {
  string data = "--myboundary\r\nContent-Type: image/jpeg\r\nContent-Length: "
    + boost::lexical_cast<string>(msg->data.size()) + "\r\n\r\n";

  data.append((const char*) &(msg->data[0]), msg->data.size());

  data += "\r\n";

  const char *buf = data.c_str();
  int buf_len = data.length();

  /* Send frame to our subscribers */
  {
    boost::mutex::scoped_lock con_lock(con_mutex);

    for (list<boost::tuple<struct mg_connection*, boost::condition_variable*, boost::mutex*> >::iterator it = connections.begin();
         it != connections.end(); it++) {
      struct mg_connection *con = (*it).get<0>();
      boost::condition_variable *cond = (*it).get<1>();
      // boost::mutex *single_mutex = (*it).get<2>();

      if (mg_write(con, buf, buf_len) < buf_len) {
        it = connections.erase(it);
        cond->notify_one();
      }
    }
  }
}

int main (int argc, char **argv) {
  ros::init(argc, argv, "jpeg_streamer");

  JPEGStreamer streamer;

  ros::spin();

  return 0;
}
