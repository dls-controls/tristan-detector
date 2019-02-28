
odin_data = {
  api_version: '0.1',
  ctrl_name: 'tristan',
  current_page: '.home-view',
  adapter_list: [],
  adapter_objects: {},
  ctrl_connected: false,
  fp_connected: [false,false,false,false]
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
    ctrl_command('arm');
  });

  $('#detector-run-cmd').click(function(){
    ctrl_command('run');
  });

  $('#detector-stop-cmd').click(function(){
    ctrl_command('stop');
  });

  $('#detector-abort-cmd').click(function(){
    ctrl_command('abort');
  });

  $('#detector-dac-scan-cmd').click(function(){
    ctrl_command('dac_scan');
  });

  $('#detector-config-put-cmd').click(function(){
    detector_json_put_command();
  });

  $('#detector-config-get-cmd').click(function(){
    detector_json_get_command();
  });

  $('#fp-debug-level').change(function(){
    fp_debug_command();
    fr_debug_command();
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

  $('#fr-reset-cmd').click(function(){
      fr_reset_statistics();
      fp_reset_statistics();
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
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/exposure/' + set_value, process_cmd_response);
}

function update_frames() {
    set_value = $('#set-hw-frames').val();
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/frames/' + set_value, process_cmd_response);
}

function update_frames_per_trigger() {
    set_value = $('#set-hw-frames-per-trigger').val();
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/frames_per_trigger/' + set_value, process_cmd_response);
}

function update_mode() {
    set_value = $('#set-hw-mode').find(":selected").text();
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/mode/' + set_value, process_cmd_response);
    fp_mode_command(set_value);
}

function update_profile() {
    set_value = $('#set-hw-profile').find(":selected").text();
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/profile/' + set_value, process_cmd_response);
}

function ctrl_command(command) {
    $.put('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/command/' + command, process_cmd_response);
}

function detector_json_put_command()
{
    try
    {
        value = JSON.stringify(JSON.parse($('#engineering-cmd').val()));
        $.ajax({
            url: '/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/engineering_put',
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
            url: '/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/engineering_get',
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

function send_fr_command(command, params)
{
    $.ajax({
        url: '/api/' + odin_data.api_version + '/fr/config/' + command,
        type: 'PUT',
        dataType: 'json',
        data: params,
        headers: {'Content-Type': 'application/json',
            'Accept': 'application/json'},
        success: process_cmd_response,
        async: false
    });
}

function fr_reset_statistics()
{
    $.ajax({
        url: '/api/' + odin_data.api_version + '/fr/command/reset_statistics',
        type: 'PUT',
        dataType: 'json',
        headers: {'Content-Type': 'application/json',
            'Accept': 'application/json'},
        success: process_cmd_response,
        async: false
    });
}

function fp_reset_statistics()
{
    $.ajax({
        url: '/api/' + odin_data.api_version + '/fp/command/reset_statistics',
        type: 'PUT',
        dataType: 'json',
        headers: {'Content-Type': 'application/json',
            'Accept': 'application/json'},
        success: process_cmd_response,
        async: false
    });
}

/*
function fp_configure_command() {
//    alert("HERE!");
    send_fp_command('meta_endpoint','tcp://*:5558');
    send_fp_command('fr_setup/0', JSON.stringify({
        "fr_release_cnxn": "tcp://127.0.0.1:5002",
        "fr_ready_cnxn": "tcp://127.0.0.1:5001"
    }));
    send_fp_command('fr_setup/1', JSON.stringify({
                "fr_release_cnxn": "tcp://127.0.0.1:5012",
                "fr_ready_cnxn": "tcp://127.0.0.1:5011"
                        }));
    send_fp_command('plugin', JSON.stringify({
        "load": {
            "library": "/home/gnx91527/work/tristan/LATRD/prefix/lib/libLATRDProcessPlugin.so",
//            "library": "/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib/libLATRDProcessPlugin.so",
            "index": "latrd",
            "name": "LATRDProcessPlugin"
        }
    }));
    send_fp_command('plugin', JSON.stringify({
        "load": {
//            "library": "/dls_sw/work/tools/RHEL6-x86_64/odin-data/prefix/lib/libHdf5Plugin.so",
            "library": "/home/gnx91527/work/tristan/odin-data/prefix/lib/libHdf5Plugin.so",
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
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "cue_timestamp_zero": {
                "datatype": 3,
                "chunks": [524288]
            }
        }
    }));
    send_fp_command('hdf', JSON.stringify({
        "dataset": {
            "cue_id": {
                "datatype": 1,
                "chunks": [524288]
            }
        }
    }));
}
*/

function fp_debug_command() {
    send_fp_command('debug_level', $('#fp-debug-level').val());
}

function fr_debug_command() {
    send_fr_command('debug_level', $('#fp-debug-level').val());
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
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/username', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-username').html(response.username);
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/start_time', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-start-time').html(response.start_time);
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/up_time', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-up-time').html(response.up_time);
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/endpoint', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-endpoint').html(response.endpoint);
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/status/connected', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-connected').html(led_html(''+response['value'], 'green', 26));
        if (!response['value']){
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
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/status/detector/software_version', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-version').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/status/detector/version_check', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-version-good').html(led_html(response['value'], 'green', 26));
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/status/detector/description', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-description').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/exposure', function(response) {
        //alert(JSON.stringify(response));
        if (odin_data.ctrl_connected){
            $('#detector-hw-exposure').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/frames', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-frames').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/frames_per_trigger', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-frames-per-trigger').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/mode', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-mode').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/config/profile', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-profile').html(response['value']);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/' + odin_data.ctrl_name + '/status/state', function(response) {
        if (odin_data.ctrl_connected){
            $('#detector-hw-state').html(response['value']);
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
        $('#fp-connected-1').html(led_html(response['value'][0], 'green', 26));
        $('#fp-connected-2').html(led_html(response['value'][1], 'green', 26));
        $('#fp-connected-3').html(led_html(response['value'][2], 'green', 26));
        $('#fp-connected-4').html(led_html(response['value'][3], 'green', 26));
        $('#fp-connected-5').html(led_html(response['value'][4], 'green', 26));
        $('#fp-connected-6').html(led_html(response['value'][5], 'green', 26));
        $('#fp-connected-7').html(led_html(response['value'][6], 'green', 26));
        $('#fp-connected-8').html(led_html(response['value'][7], 'green', 26));
        if (response['value'][0] === false){
            odin_data.fp_connected[0] = false;
            $('#fp-hdf-writing').html('');
            $('#fp-hdf-file-path').html('');
            $('#fp-hdf-processes').html('');
            $('#fp-hdf-rank').html('');
            $('#set-fp-filename').val('');
            $('#set-fp-path').val('');
        } else {
            odin_data.fp_connected[0] = true;
        }
        if (response['value'][1] === false){
            odin_data.fp_connected[1] = false;
        } else {
            odin_data.fp_connected[1] = true;
        }
        if (response['value'][2] === false){
            odin_data.fp_connected[2] = false;
        } else {
            odin_data.fp_connected[2] = true;
        }
        if (response['value'][3] === false){
            odin_data.fp_connected[3] = false;
        } else {
            odin_data.fp_connected[3] = true;
        }
        if (response['value'][4] === false){
            odin_data.fp_connected[4] = false;
        } else {
            odin_data.fp_connected[4] = true;
        }
        if (response['value'][5] === false){
            odin_data.fp_connected[5] = false;
        } else {
            odin_data.fp_connected[5] = true;
        }
        if (response['value'][6] === false){
            odin_data.fp_connected[6] = false;
        } else {
            odin_data.fp_connected[6] = true;
        }
        if (response['value'][7] === false){
            odin_data.fp_connected[7] = false;
        } else {
            odin_data.fp_connected[7] = true;
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/status/hdf', function(response) {
//        alert(response['value']);
        
        if (odin_data.fp_connected[0] == true){
          update_fp_data(1, response['value'][0]);
        }
        if (odin_data.fp_connected[1] == true){
          update_fp_data(2, response['value'][1]);
        }
        if (odin_data.fp_connected[2] == true){
          update_fp_data(3, response['value'][2]);
        }
        if (odin_data.fp_connected[3] == true){
          update_fp_data(4, response['value'][3]);
        }
        if (odin_data.fp_connected[4] == true){
          update_fp_data(5, response['value'][4]);
        }
        if (odin_data.fp_connected[5] == true){
          update_fp_data(6, response['value'][5]);
        }
        if (odin_data.fp_connected[6] == true){
          update_fp_data(7, response['value'][6]);
        }
        if (odin_data.fp_connected[7] == true){
          update_fp_data(8, response['value'][7]);
        }
//        $('#fp-processes-1').html('' + response['value'][0].processes);
//        $('#fp-processes-2').html('' + response['value'][1].processes);
//        $('#fp-processes-3').html('' + response['value'][2].processes);
//        $('#fp-processes-4').html('' + response['value'][3].processes);
//        $('#fp-rank-1').html('' + response['value'][0].rank);
//        $('#fp-rank-2').html('' + response['value'][1].rank);
//        $('#fp-rank-3').html('' + response['value'][2].rank);
//        $('#fp-rank-4').html('' + response['value'][3].rank);
//        alert(response['value'][0].rank);
//        if (odin_data.fp_connected){
//            if (response['value'] == ""){
//                $('#fp-hdf-writing').html('Not Loaded');
//                $('#fp-hdf-file-path').html('Not Loaded');
//                $('#fp-hdf-processes').html('Not Loaded');
//                $('#fp-hdf-rank').html('Not Loaded');
//            } else {
//                //alert(response['value'][0]);
//                $('#fp-hdf-writing').html(led_html(response['value'][0].writing, 'green', 26));
//                $('#fp-hdf-file-path').html('' + response['value'][0].file_path + response['value'][0].file_name);
//                $('#fp-hdf-processes').html('' + response['value'][0].processes);
//                $('#fp-hdf-rank').html('' + response['value'][0].rank);
//                $('#fp-hdf-written').html('' + response['value'][0].frames_written);
//            }
//        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/status/latrd', function(response) {
//        alert(response['value']);

        if (odin_data.fp_connected[0] == true){
            update_fp_latrd(1, response['value'][0]);
        }
        if (odin_data.fp_connected[1] == true){
            update_fp_latrd(2, response['value'][1]);
        }
        if (odin_data.fp_connected[2] == true){
            update_fp_latrd(3, response['value'][2]);
        }
        if (odin_data.fp_connected[3] == true){
            update_fp_latrd(4, response['value'][3]);
        }
        if (odin_data.fp_connected[4] == true){
            update_fp_latrd(5, response['value'][4]);
        }
        if (odin_data.fp_connected[5] == true){
            update_fp_latrd(6, response['value'][5]);
        }
        if (odin_data.fp_connected[6] == true){
            update_fp_latrd(7, response['value'][6]);
        }
        if (odin_data.fp_connected[7] == true){
            update_fp_latrd(8, response['value'][7]);
        }
    });
    $.getJSON('/api/' + odin_data.api_version + '/fp/config/latrd', function(response) {
        //alert(response['value']);
        if (odin_data.fp_connected[0]){
            if (response['value'][0] == ""){
                $('#fp-raw-mode-1').html('Not Loaded');
            } else {
                $('#fp-op-mode-1').html(response['value'][0].mode);
                if (response['value'][0].raw_mode == "0") {
                    $('#fp-raw-mode-1').html('Process');
                    $('#fp-mode-row').show();
                } else {
                    $('#fp-raw-mode-1').html('Raw');
                    $('#fp-mode-row').hide();
                }
            }
        }
        if (odin_data.fp_connected[1]){
            if (response['value'][1] == ""){
                $('#fp-raw-mode-2').html('Not Loaded');
            } else {
                $('#fp-op-mode-2').html(response['value'][1].mode);
                if (response['value'][1].raw_mode == "0") {
                    $('#fp-raw-mode-2').html('Process');
                } else {
                    $('#fp-raw-mode-2').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[2]){
            if (response['value'][2] == ""){
                $('#fp-raw-mode-3').html('Not Loaded');
            } else {
                $('#fp-op-mode-3').html(response['value'][2].mode);
                if (response['value'][2].raw_mode == "0") {
                    $('#fp-raw-mode-3').html('Process');
                } else {
                    $('#fp-raw-mode-3').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[3]){
            if (response['value'][3] == ""){
                $('#fp-raw-mode-4').html('Not Loaded');
            } else {
                $('#fp-op-mode-4').html(response['value'][3].mode);
                if (response['value'][3].raw_mode == "0") {
                    $('#fp-raw-mode-4').html('Process');
                } else {
                    $('#fp-raw-mode-4').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[4]){
            if (response['value'][4] == ""){
                $('#fp-raw-mode-5').html('Not Loaded');
            } else {
                $('#fp-op-mode-5').html(response['value'][4].mode);
                if (response['value'][4].raw_mode == "0") {
                    $('#fp-raw-mode-5').html('Process');
                } else {
                    $('#fp-raw-mode-5').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[5]){
            if (response['value'][5] == ""){
                $('#fp-raw-mode-6').html('Not Loaded');
            } else {
                $('#fp-op-mode-6').html(response['value'][5].mode);
                if (response['value'][5].raw_mode == "0") {
                    $('#fp-raw-mode-6').html('Process');
                } else {
                    $('#fp-raw-mode-6').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[6]){
            if (response['value'][6] == ""){
                $('#fp-raw-mode-7').html('Not Loaded');
            } else {
                $('#fp-op-mode-7').html(response['value'][6].mode);
                if (response['value'][6].raw_mode == "0") {
                    $('#fp-raw-mode-7').html('Process');
                } else {
                    $('#fp-raw-mode-7').html('Raw');
                }
            }
        }
        if (odin_data.fp_connected[7]){
            if (response['value'][7] == ""){
                $('#fp-raw-mode-8').html('Not Loaded');
            } else {
                $('#fp-op-mode-8').html(response['value'][7].mode);
                if (response['value'][7].raw_mode == "0") {
                    $('#fp-raw-mode-8').html('Process');
                } else {
                    $('#fp-raw-mode-8').html('Raw');
                }
            }
        }
    });
}

function update_fp_data(index, data){
    $('#fp-processes-'+index).html('' + data.processes);
    $('#fp-rank-'+index).html('' + data.rank);
    $('#fp-writing-'+index).html(led_html(data.writing, 'green', 26));
    $('#fp-written-'+index).html('' + data.frames_written);
}

function update_fp_latrd(index, data){
    $('#fp-invalid-pkt-'+index).html('' + data.invalid_packets);
    $('#fp-pkts-processed-'+index).html('' + data.processed_jobs);
    $('#fp-job-q-'+index).html('' + data.job_queue);
    $('#fp-result-q-'+index).html('' + data.results_queue);
    $('#fp-proc-frames-'+index).html('' + data.processed_frames);
    $('#fp-out-frames-'+index).html('' + data.output_frames);
}

function update_fr_status() {
    $.getJSON('/api/' + odin_data.api_version + '/fr/status/buffers', function (response) {
        //alert(response['value'][0].empty);
        $('#fr-empty-buffers-1').html(response['value'][0].empty);
        $('#fr-empty-buffers-2').html(response['value'][1].empty);
        $('#fr-empty-buffers-3').html(response['value'][2].empty);
        $('#fr-empty-buffers-4').html(response['value'][3].empty);
        $('#fr-empty-buffers-5').html(response['value'][4].empty);
        $('#fr-empty-buffers-6').html(response['value'][5].empty);
        $('#fr-empty-buffers-7').html(response['value'][6].empty);
        $('#fr-empty-buffers-8').html(response['value'][7].empty);
    });
    $.getJSON('/api/' + odin_data.api_version + '/fr/status/decoder/packets', function (response) {
        //alert(response['value']);
        $('#fr-packets-1').html(response['value'][0]);
        $('#fr-packets-2').html(response['value'][1]);
        $('#fr-packets-3').html(response['value'][2]);
        $('#fr-packets-4').html(response['value'][3]);
        $('#fr-packets-5').html(response['value'][4]);
        $('#fr-packets-6').html(response['value'][5]);
        $('#fr-packets-7').html(response['value'][6]);
        $('#fr-packets-8').html(response['value'][7]);
    });
    $.getJSON('/api/' + odin_data.api_version + '/fr/status/connected', function(response) {
        //alert(response['value']);
        $('#fr-connected-1').html(led_html(response['value'][0], 'green', 26));
        $('#fr-connected-2').html(led_html(response['value'][1], 'green', 26));
        $('#fr-connected-3').html(led_html(response['value'][2], 'green', 26));
        $('#fr-connected-4').html(led_html(response['value'][3], 'green', 26));
        $('#fr-connected-5').html(led_html(response['value'][4], 'green', 26));
        $('#fr-connected-6').html(led_html(response['value'][5], 'green', 26));
        $('#fr-connected-7').html(led_html(response['value'][6], 'green', 26));
        $('#fr-connected-8').html(led_html(response['value'][7], 'green', 26));
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
