
odin_data = {
  api_version: '0.1',
  current_page: '.home-view',
  adapter_list: [],
  adapter_objects: {},
  ctrl_connected: false
  };


String.prototype.replaceAll = function(search, replacement) {
    var target = this;
    return target.replace(new RegExp(search, 'g'), replacement);
};

$.put = function(url, data, callback, type)
{
  if ( $.isFunction(data) ){
    type = type || callback,
    callback = data,
    data = {}
  }

  return $.ajax({
    url: url,
    type: 'PUT',
    success: callback,
    data: data,
    contentType: type
  });
}

function render(url)
{
  // This function decides what type of page to show
  // depending on the current url hash value.
  // Get the keyword from the url.
  var temp = "." + url.split('/')[1];
  // Hide whatever page is currently shown.
  $(".page").hide();

  // Show the new page
  $(temp).show();
  odin_data.current_page = temp;

}


$( document ).ready(function() 
{
  $("#fp-tabs").tabs();
  update_api_version();
  update_api_adapters();
  render(decodeURI(window.location.hash));

  setInterval(update_api_version, 5000);
  setInterval(update_detector_status, 1000);
  setInterval(update_fp_status, 1000);
  setInterval(update_fr_status, 1000);

  $('#set-hw-exposure').change(function(){
    update_exposure();
  });

  $('#set-hw-frames').change(function(){
    update_frames();
  });

  $('#set-hw-frames-per-trigger').change(function(){
    update_frames_per_trigger();
  });

  $('#set-hw-mode').change(function(){
    update_mode();
  });

  $('#set-hw-profile').change(function(){
    update_profile();
  });

  $('#detector-arm-cmd').click(function(){
    ctrl_command('Arm');
  });

  $('#detector-run-cmd').click(function(){
    ctrl_command('Run');
  });

  $('#detector-stop-cmd').click(function(){
    ctrl_command('Stop');
  });

  $('#detector-abort-cmd').click(function(){
    ctrl_command('Abort');
  });

  $('#detector-config-put-cmd').click(function(){
    detector_json_put_command();
  });

  $('#detector-config-get-cmd').click(function(){
    detector_json_get_command();
  });

  $('#fp-config-cmd').click(function(){
    fp_configure_command();
  });

  $('#fp-start-cmd').click(function(){
    fp_start_command();
  });

  $('#fp-stop-cmd').click(function(){
    fp_stop_command();
  });

  $('#fp-config-raw-mode-on').click(function(){
    fp_raw_mode_command();
  });

  $('#fp-config-raw-mode-off').click(function(){
    fp_process_mode_command();
  });

  $(window).on('hashchange', function(){
    // On every hash change the render function is called with the new hash.
	// This is how the navigation of the app is executed.
	render(decodeURI(window.location.hash));
  });
});

function process_cmd_response(response)
{
}

function update_exposure() {
    set_value = $('#set-hw-exposure').val();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Exposure/' + set_value, process_cmd_response);
}

function update_frames() {
    set_value = $('#set-hw-frames').val();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Frames/' + set_value, process_cmd_response);
}

function update_frames_per_trigger() {
    set_value = $('#set-hw-frames-per-trigger').val();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Frames_Per_Trigger/' + set_value, process_cmd_response);
}

function update_mode() {
    set_value = $('#set-hw-mode').find(":selected").text();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Mode/' + set_value, process_cmd_response);
    fp_mode_command(set_value);
}

function update_profile() {
    set_value = $('#set-hw-profile').find(":selected").text();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Profile/' + set_value, process_cmd_response);
}

function ctrl_command(command) {
    $.put('/api/' + odin_data.api_version + '/ctrl/command/' + command, process_cmd_response);
}

function detector_json_put_command()
{
    try
    {
        value = JSON.stringify(JSON.parse($('#engineering-cmd').val()));
        $.ajax({
            url: '/api/' + odin_data.api_version + '/ctrl/engineering_put',
            type: 'PUT',
            dataType: 'json',
            data: value,
            headers: {'Content-Type': 'application/json',
                      'Accept': 'application/json'},
            success: function(response){
                $('#engineering-rsp').val(JSON.stringify(JSON.parse(response['reply'])));
            },
            async: false
        });
    }
    catch(err)
    {
        alert("Failed to submit JSON PUT command.\nError: " + err.message);
    }
}

function detector_json_get_command()
{
    try
    {
        value = JSON.stringify(JSON.parse($('#engineering-cmd').val()));
        $.ajax({
            url: '/api/' + odin_data.api_version + '/ctrl/engineering_get',
            type: 'PUT',
            dataType: 'json',
            data: value,
            headers: {'Content-Type': 'application/json',
                      'Accept': 'application/json'},
            success: function(response){
                $('#engineering-rsp').val(JSON.stringify(JSON.parse(response['reply'])));
            },
            async: false
        });
    }
    catch(err)
    {
        alert("Failed to submit JSON GET command.\nError: " + err.message);
    }
}

function send_fp_command(command, params)
{
    $.ajax({
        url: '/api/' + odin_data.api_version + '/fp/config/' + command,
        type: 'PUT',
        dataType: 'json',
        data: params,
        headers: {'Content-Type': 'application/json',
                  'Accept': 'application/json'},
        success: process_cmd_response,
        async: false
    });
}

function fp_configure_command() {
//    alert("HERE!");
    send_fp_command('meta_endpoint','tcp://*:5558');
    send_fp_command('fr_setup', JSON.stringify({
        "fr_release_cnxn": "tcp://127.0.0.1:5002",
        "fr_ready_cnxn": "tcp://127.0.0.1:5001"
    }));
    send_fp_command('plugin', JSON.stringify({
        "load": {
            "library": "/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib/libLATRDProcessPlugin.so",
            "index": "latrd",
            "name": "LATRDProcessPlugin"
        }
    }));
    send_fp_command('plugin', JSON.stringify({
        "load": {
            "library": "/dls_sw/prod/tools/RHEL6-x86_64/odin-data/0-4-0dls2/prefix/lib/libHdf5Plugin.so",
            "index": "hdf",
            "name": "FileWriterPlugin"
        }
    }));
    send_fp_command('plugin', JSON.stringify({
        "connect": {
            "index": "latrd",
            "connection": "frame_receiver"
        }
    }));
    send_fp_command('plugin', JSON.stringify({
        "connect": {
            "index": "hdf",
            "connection": "latrd"
        }
    }));
    send_fp_command('latrd', JSON.stringify({
        "process": {
            "number": 1,
            "rank": 0
        },
        "sensor": {
            "width": 2048,
            "height": 512
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "raw_data": {
                "datatype": 3,
                "chunks": [524288]
            }
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "event_id": {
                "datatype": 2,
                "chunks": [524288]
            }
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "event_time_offset": {
                "datatype": 3,
                "chunks": [524288]
            }
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "event_energy": {
                "datatype": 2,
                "chunks": [524288]
            }
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "image": {
                "datatype": 1,
                "dims": [512, 2048],
                "chunks": [1, 512, 2048]
            }
        }
    }));
}

function fp_mode_command(mode) {
    send_fp_command('latrd', JSON.stringify({
        "mode": mode.toLowerCase()
    }));
}

function fp_start_command() {
    send_fp_command('hdf', JSON.stringify({
        "file": {
            "name": $('#set-fp-filename').val(),
            "path": $('#set-fp-path').val()
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "write": true
    }));
}

function fp_raw_mode_command() {
    send_fp_command('latrd', JSON.stringify({
        "raw_mode": 1
    }));
}

function fp_process_mode_command() {
    send_fp_command('latrd', JSON.stringify({
        "raw_mode": 0
    }));
}

function fp_stop_command() {
    send_fp_command('hdf', JSON.stringify({
        "write": false
    }));
}

function update_api_version() {

    $.getJSON('/api', function(response) {
        $('#api-version').html(response.api_version);
        odin_data.api_version = response.api_version;
    });
}

function update_api_adapters() {
    $.getJSON('/api/' + odin_data.api_version + '/adapters/', function(response) {
        odin_data.adapter_list = response.adapters;
        adapter_list_html = response.adapters.join(", ");
        $('#api-adapters').html(adapter_list_html);
        //update_adapter_objects();
    });
}

function update_detector_status() {
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/username', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-username').html(response.username);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/start_time', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-start-time').html(response.start_time);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/up_time', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-up-time').html(response.up_time);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/endpoint', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-endpoint').html(response.endpoint);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Connected', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-connected').html(led_html(''+response['status/Connected'], 'green', 26));
        if (!response['status/Connected']){
            $('#set-hw-exposure').val('');
            $('#set-hw-frames').val('');
            $('#set-hw-frames-per-trigger').val('');
            $('#detector-hw-version').html('');
            $('#detector-hw-version-good').html('');
            $('#detector-hw-description').html('');
            $('#detector-hw-exposure').html('');
            $('#detector-hw-frames').html('');
            $('#detector-hw-frames-per-trigger').html('');
            $('#detector-hw-mode').html('');
            $('#detector-hw-profile').html('');
            odin_data.ctrl_connected = false;
        } else {
            odin_data.ctrl_connected = true;
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Software_Version', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-version').html(response['status/Software_Version']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Version_Check', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-version-good').html(led_html(response['status/Version_Check'], 'green', 26));
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Description', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-description').html(response['status/Description']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Exposure', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-exposure').html(response['config/Exposure']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Frames', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-frames').html(response['config/Frames']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Frames_Per_Trigger', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-frames-per-trigger').html(response['config/Frames_Per_Trigger']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Mode', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-mode').html(response['config/Mode']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Profile', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-profile').html(response['config/Profile']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/State', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-state').html(response['status/State']);
        }
    });
}

function update_fp_status() {
    $.getJSON('/api/' + odin_data.api_version + '/fp/endpoint', function(response) {
//        alert(response.endpoint);
        $('#fp-endpoint').html(response.endpoint);
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/status/connected', function(response) {
        //alert(response['value']);
        $('#fp-connected').html(led_html(response['value'], 'green', 26));
        if (response['value'] === 'false'){
            odin_data.fp_connected = false;
            $('#fp-hdf-writing').html('');
            $('#fp-hdf-file-path').html('');
            $('#fp-hdf-processes').html('');
            $('#fp-hdf-rank').html('');
            $('#set-fp-filename').val('');
            $('#set-fp-path').val('');
        } else {
            odin_data.fp_connected = true;
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/status/hdf', function(response) {
        //alert(response['value']);
        if (odin_data.fp_connected){
            if (response['value'] == ""){
                $('#fp-hdf-writing').html('Not Loaded');
                $('#fp-hdf-file-path').html('Not Loaded');
                $('#fp-hdf-processes').html('Not Loaded');
                $('#fp-hdf-rank').html('Not Loaded');
            } else {
                //alert(response['value'][0]);
                $('#fp-hdf-writing').html(led_html(response['value'][0].writing, 'green', 26));
                $('#fp-hdf-file-path').html('' + response['value'][0].file_path + response['value'][0].file_name);
                $('#fp-hdf-processes').html('' + response['value'][0].processes);
                $('#fp-hdf-rank').html('' + response['value'][0].rank);
                $('#fp-hdf-written').html('' + response['value'][0].frames_written);
            }
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/config/latrd', function(response) {
        //alert(response['value']);
        if (odin_data.fp_connected){
            if (response['value'] == ""){
                $('#fp-raw-mode').html('Not Loaded');
            } else {
//                alert(response['value'][0].raw_mode);
                $('#fp-op-mode').html(response['value'][0].mode);
                if (response['value'][0].mode == "count"){
                    $('#fp-raw-row').hide();
                } else {
                    $('#fp-raw-row').show();
                }
                if (response['value'][0].raw_mode == "0") {
                    $('#fp-raw-mode').html('Process');
                } else {
                    $('#fp-raw-mode').html('Raw');
                }
            }
        }
    });
}

function update_fr_status() {
    $.getJSON('/api/' + odin_data.api_version + '/fr/status/buffers', function (response) {
        //alert(response['value'][0].empty);
        $('#fr-empty-buffers').html(response['value'][0].empty);
    });
}

function led_html(value, colour, width)
{
  var html_text = "<img width=" + width + "px src=img/";
  if (value == 'true' || value === true){
    html_text +=  colour + "-led-on";
  } else {
    html_text += "led-off";
  }
  html_text += ".png></img>";
  return html_text;
}
