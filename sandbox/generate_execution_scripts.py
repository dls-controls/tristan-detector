import os
import argparse
import json

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--number", type=int, default=4, help="Number of FR and FP pairs to create")
    parser.add_argument("-o", "--outdir", default="./data/", help="Path where the output files will be written")
    args = parser.parse_args()
    return args


def generate_fp_json(out_dir, total, number):
    ready_connection = 5001 + (number * 10)
    release_connection = 5002 + (number * 10)
    json_dict = [
  {
    "fr_setup": {
      "fr_ready_cnxn": "tcp://127.0.0.1:{}".format(ready_connection),
      "fr_release_cnxn": "tcp://127.0.0.1:{}".format(release_connection)
    }
  },
  {
    "plugin": {
      "load": {
        "index": "hdf",
        "name": "FileWriterPlugin",
        "library": "/home/gnx91527/work/tristan/odin-data/prefix/lib/libHdf5Plugin.so"
      }
    }
  },
  {
    "plugin": {
      "load": {
        "index": "latrd",
        "name": "LATRDProcessPlugin",
        "library": "/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib/libLATRDProcessPlugin.so"
      }
    }
  },
  {
    "plugin": {
      "connect": {
        "index": "latrd",
        "connection": "frame_receiver"
      }
    }
  },
  {
    "plugin": {
      "connect": {
        "index": "hdf",
        "connection": "latrd"
      }
    }
  },
  {
    "latrd": {
      "process": {
        "number": total,
        "rank": number
      },
      "sensor": {
        "width": 2048,
        "height": 512
      }
    }
  },
  {
    "hdf": {
      "process": {
        "number": total,
        "rank": number
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "raw_data": {
          "datatype": 3,
          "chunks": [524288]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "event_id": {
          "datatype": 2,
          "chunks": [524288]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "event_time_offset": {
          "datatype": 3,
          "chunks": [524288]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "event_energy": {
          "datatype": 2,
          "chunks": [524288]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "image": {
          "datatype": 1,
          "dims": [512, 2048],
          "chunks": [1, 512, 2048]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "cue_timestamp_zero": {
          "datatype": 3,
          "chunks": [524288]
        }
      }
    }
  },
  {
    "hdf": {
      "dataset": {
        "cue_id": {
          "datatype": 1,
          "chunks": [524288]
        }
      }
    }
  }
]
    with open(os.path.join(out_dir, 'fp{}.json'.format(number+1)), 'w') as outfile:
        json.dump(json_dict, outfile, indent=4)

    os.chmod(os.path.join(out_dir, 'fp{}.json'.format(number+1)), 0o666)

def generate_fp_script(out_dir, total, number):
  ctrl_connection = 5004 + (number * 10)
  fp_json=os.path.join(out_dir, 'fp{}.json'.format(number+1))
  script_string = '#!/bin/bash\n\
\n\
/home/gnx91527/work/tristan/odin-data/prefix/bin/frameProcessor --ctrl=tcp://0.0.0.0:{} --json_file={} \
--logconfig=/dls_sw/work/tools/RHEL6-x86_64/LATRD/lab29/log4cxx.xml "$@"\n\n'.format(ctrl_connection, fp_json)

  with open(os.path.join(out_dir, 'tristan_processor_{}'.format(number+1)), 'w') as outfile:
    outfile.write(script_string)

  os.chmod(os.path.join(out_dir, 'tristan_processor_{}'.format(number+1)), 0o777)


def generate_fr_script(out_dir, total, number):
  ctrl_connection = 5000 + (number * 10)
  port_number = 61649 + number
  ready_connection = 5001 + (number * 10)
  release_connection = 5002 + (number * 10)
  memory = 10485760000 / total
  script_string = '#!/bin/bash\n\
\n\
BUFFER_NAME="FrameReceiverBuffer1"\n\
BUFFER_STRING="$USER$BUFFER_NAME"\n\
/dls_sw/prod/tools/RHEL6-x86_64/odin-data/0-6-0/prefix/bin/frameReceiver \
--path /dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib --ctrl=tcp://*:{} \
--port={} --sharedbuf=$BUFFER_STRING --ready=tcp://*:{} --release=tcp://*:{} \
-t LATRD --rxbuffer=300000000 --sharedbuf=$BUFFER_STRING -m {} \
--logconfig=/dls_sw/work/tools/RHEL6-x86_64/LATRD/lab29/log4cxx.xml "$@"\n\n'.format(ctrl_connection,
                                                                                     port_number,
                                                                                     ready_connection,
                                                                                     release_connection,
                                                                                     memory)
  with open(os.path.join(out_dir, 'tristan_receiver_{}'.format(number+1)), 'w') as outfile:
    outfile.write(script_string)

  os.chmod(os.path.join(out_dir, 'tristan_receiver_{}'.format(number+1)), 0o777)


def generate_odin_scripts(out_dir, total):
  fp_endpoint = ''
  fr_endpoint = ''
  for index in range(0, total):
    fp_endpoint+='127.0.0.1:{},'.format(5004 + (index * 10))
    fr_endpoint+='127.0.0.1:{},'.format(5000 + (index * 10))
  fp_endpoint = fp_endpoint[:-1]
  fr_endpoint = fr_endpoint[:-1]

  script_string = '[server]\n\
debug_mode = 1\n\
http_port  = 8888\n\
http_addr  = 0.0.0.0\n\
static_path = /dls_sw/work/tools/RHEL6-x86_64/LATRD/control/latrd/odin/static\n\
adapters   = ctrl,fr,fp\n\
\n\
[tornado]\n\
logging = debug\n\
\n\
[adapter.ctrl]\n\
module = latrd.detector.tristan_control_adapter.TristanControlAdapter\n\
endpoint = tcp://192.168.0.34:5000\n\
#endpoint = tcp://192.168.1.2:5000\n\
#endpoint = tcp://127.0.0.1:5100\n\
firmware = 0.0.1\n\
\n\
[adapter.fr]\n\
module = odin_data.frame_processor_adapter.OdinDataAdapter\n\
endpoints = {}\n\
update_interval = 0.5\n\
\n\
[adapter.fp]\n\
module = odin_data.frame_processor_adapter.FrameProcessorAdapter\n\
endpoints = {}\n\
update_interval = 0.5\n\n'.format(fr_endpoint, fp_endpoint)
  with open(os.path.join(out_dir, 'tristan_odin.cfg'), 'w') as outfile:
    outfile.write(script_string)

  os.chmod(os.path.join(out_dir, 'tristan_odin.cfg'), 0o666)

  config_path = os.path.join(out_dir, 'tristan_odin.cfg')

  script_string='export PYTHONPATH=/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib/python2.7/site-packages/latrd-0.1.0-py2.7.egg:\
/dls_sw/work/tools/RHEL6-x86_64/odin-data/prefix/lib/python2.7/site-packages/odin_data-0_5_0dls1_147.g10c558a.dirty-py2.7.egg\n\
/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/bin/latrd_odin --config={}\n\n'.format(config_path)
  with open(os.path.join(out_dir, 'odin_control_server_{}'.format(total)), 'w') as outfile:
    outfile.write(script_string)

  os.chmod(os.path.join(out_dir, 'odin_control_server_{}'.format(total)), 0o777)



def main():
    args = options()

    directory = os.path.dirname(args.outdir)
    directory = os.path.abspath(directory)
    if not os.path.exists(directory):
        os.makedirs(directory)

    for index in range(0, args.number):
      generate_fp_json(directory, args.number, index)
      generate_fp_script(directory, args.number, index)
      generate_fr_script(directory, args.number, index)
    generate_odin_scripts(directory, args.number)

if __name__ == "__main__":
    main()
