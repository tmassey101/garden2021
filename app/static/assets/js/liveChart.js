<script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.13.0/moment.min.js"></script>
<script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
<script>
    
    function formatTimestamp(timestamp){
        return ts = moment(timestamp)
    }

    const labels = {{graph_x|safe}};
    const y1 = {{graph_y1|safe}};
    const y2 = {{graph_y2|safe}};

    const color1 = 'rgb(255, 99, 132, 1)';
    const color2 = 'rgb(54, 162, 235, 1)';
    const bordercolor1 = 'rgb(255, 99, 132, 0.5)';
    const bordercolor2 = 'rgb(54, 162, 235, 0.5)';

    const data1 = []
    const data2 = []
    const x1 = []
    const x2 = []

    for(i = 0; i < labels.length; i++){
        ts = formatTimestamp(labels[i]);
        console.log(labels[i]);
        data1.push(y1[i]);
        data2.push(y2[i]);
        x1.push[labels[i]];
        x2.push[labels[i]];
    }

    console.log(data1);
    console.log(x1);
    console.log(data2);
    console.log(x2);
    
	var trace1 = {
    x: labels,
    y: data1,
    name: 'Temperature',
    type: 'scatter',
    mode: 'lines',
    line: {
        color: 'rgb(219, 64, 82)',
        width: 3
    }
    };

    var trace2 = {
    x: labels,
    y: data2,
    name: 'Moisture',
    yaxis: 'y2',
    type: 'scatter',
    mode: 'lines',
    line: {
        color: 'rgb(55, 128, 191)',
        width: 1
    }
    };

    var data = [trace1, trace2];

    var layout = {
    title: 'Soil Sensor Readings',
    yaxis: {title: 'Temperature'},
    yaxis2: {
        title: 'Moisture Content',
        titlefont: {color: 'rgb(148, 103, 189)'},
        tickfont: {color: 'rgb(148, 103, 189)'},
        overlaying: 'y',
        side: 'right'
    }
    };

    Plotly.newPlot('tester', data, layout);

    </script>