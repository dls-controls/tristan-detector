
odin_data = {
  api_version: '0.1',
  current_page: '.home-view',
  adapter_list: [],
  adapter_objects: {}
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


$( document ).ready(function() 
{
  $("#fp-tabs").tabs();
  update_api_version();
  update_api_adapters();

  setInterval(update_api_version, 5000);
  setInterval(update_detector_status, 1000);

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
}

function update_profile() {
    set_value = $('#set-hw-profile').find(":selected").text();
    $.put('/api/' + odin_data.api_version + '/ctrl/config/Profile/' + set_value, process_cmd_response);
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
        update_adapter_objects();
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
        $('#detector-hw-connected').html('' + response['status/Connected']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Software_Version', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-version').html(response['status/Software_Version']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Version_Check', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-version-good').html('' + response['status/Version_Check']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/status/Description', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-description').html(response['status/Description']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Exposure', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-exposure').html(response['config/Exposure']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Frames', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-frames').html(response['config/Frames']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Frames_Per_Trigger', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-frames-per-trigger').html(response['config/Frames_Per_Trigger']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Mode', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-mode').html(response['config/Mode']);
    });
    $.getJSON('/api/' + odin_data.api_version + '/ctrl/config/Profile', function(response) {
//        alert(response.endpoint);
        $('#detector-hw-profile').html(response['config/Profile']);
    });
}

function update_adapter_objects()
{
    // Loop over adapters and create new objects if necessary
    for (index = 0; index < odin_data.adapter_list.length; index++){
        adapter_name = odin_data.adapter_list[index];
        $.getJSON('/api/' + odin_data.api_version + '/' + adapter_name + '/module/', (function(adapter_name){
            return function(response) {
                //alert(typeof(odin_data.adapter_list));
                if (response.module == "FrameProcessorAdapter"){
                    if (!(adapter_name in odin_data.adapter_objects)){
                        fp = new FPAdapter(adapter_name);
                        //alert(adapter_name);
                        odin_data.adapter_objects[adapter_name] = fp;
                        //alert(JSON.stringify(odin_data.adapter_objects));
                    }
                }
            };
        }(adapter_name)));
    }
}

function update_adapter_status()
{
    //alert(JSON.stringify(odin_data.adapter_objects));
    for (var adapter in odin_data.adapter_objects){
        odin_data.adapter_objects[adapter].read_status();
        //adapter.read_status();
    }
}

class FPAdapter
{
    constructor(id)
    {
    this.id = id;
    this.tab_id = "tab-" + this.id;
    this.html_text = "<p>Frame Processor [" + this.id + "]</p>" +
        "<table>" +
        "<tr><td>IP Address:</td><td id='status-ip-" + this.id + "'></td></tr>" +
        "<tr><td>Port Number:</td><td id='status-port-" + this.id + "'></td></tr>" +
        "<tr><td colspan='2'><div id='status-plugins-" + this.id + "'></div></td></tr>" +
        "<tr><td><button id='status-" + this.id + "'>Request Status</button></td></tr>" +
        "<tr><td colspan='2'><pre id='status-response-" + this.id + "'></pre></td></tr>" +
        "</table>";
    //var tabs = $("#fp-tabs").tabs();
    $("#fp-tabs").find(".ui-tabs-nav").append("<li><a href='#" + this.tab_id + "'>" + this.id + "</a></li>");
    $("#fp-tabs").append("<div id='" + this.tab_id + "'>" + this.html_text + "</div>");
    $("#fp-tabs").tabs("refresh");

    this.read_connection();

    $("#status-" + this.id).click(function(){
        //alert($(this).attr("id") + " clicked the button");
        var adapter_name = $(this).attr("id").replace("status-", "");
        $.getJSON('/api/' + odin_data.api_version + '/' + adapter_name + '/status/', function(response) {
            //alert(JSON.stringify(response.params));
            if (response.params){
                if (response.params.plugins){
                    //alert(JSON.stringify(response.params.plugins.names));
                    for (index = 0; index < response.params.plugins.names.length; index++){
                        $("#status-response-" + adapter_name).text(JSON.stringify(response, undefined, 4));
                    }
                }
            }
        });
    });
    }

    read_connection()
    {
        $.getJSON('/api/' + odin_data.api_version + '/' + this.id + '/ip_address/', (function(id) {
            return function(response) {
                $("#status-ip-" + id).text(response.ip_address);
            };
        }(this.id)));
        $.getJSON('/api/' + odin_data.api_version + '/' + this.id + '/port/', (function(id) {
            return function(response) {
                $("#status-port-" + id).text(response.port);
            };
        }(this.id)));
    }
};

