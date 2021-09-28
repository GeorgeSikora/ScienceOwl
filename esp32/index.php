<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32</title>
    
    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.27.0/moment.min.js"></script>
    <script src='https://cdn.jsdelivr.net/npm/chart.js@2.9.3/dist/Chart.min.js'></script>
        
    <style>
    body {
        background: #111;
    }
    canvas {
        padding: 48px 32px;
    }
    </style>

</head>
<body>
    <canvas id="myChart"></canvas>
    <script>
    var ctx = document.getElementById('myChart').getContext('2d');

    /*
    const labels = [
        "2021-09-24 23:22:52",
        "2022-09-24 23:22:52",
        "2023-09-24 23:22:52",
        "2024-09-24 23:22:52",
        "2025-09-24 23:22:52",
        "2026-09-24 23:22:52",
        "2027-09-24 23:22:52",
    ];
    const data = {
        labels: labels,
        datasets: [{
            label: 'My First Dataset',
            data: [65, 59, 80, 81, 56, 55, 40],
            fill: false,
            borderColor: 'rgb(75, 192, 192)',
            tension: 0.1
        }]
    };
        
    const config = {
        type: 'line',
        data: data,
    };
    */
    </script>
</body>
</html>

<script>

function createGraph(dataText)
{
    /* SETTINGS */
    var averagingEnabled = true;
    const numberOfPoints = 100;
    
    /* FILE DATA */
    var dateCol;

    /* GRAPH DATA */
    const labels = [];
    const datasets = [];

    const lines = dataText.split(/\r?\n/);

    const headLine = lines.shift();
    const shl = headLine.split(';'); // splitted head line

    for (const col in shl)
    {
        const colName = shl[col];
        switch (colName) {
            case 'date':
                dateCol = col;
                break;
            default:
                datasets.push({
                    label: colName,
                    data: [],

                    fill: false,
                    borderColor: randomColor(),
                    tension: 0.2
                });
                break;
        }
}

    if (lines.length < numberOfPoints)
    {
        averagingEnabled = false;
    }

    if (averagingEnabled)
    {
        var avgValsLine = [];

        for (const colName of shl)
        {
            if (colName == 'date') continue;

            avgValsLine.push({
                name: colName,
                suma: 0,
                count: 0,
            });
        }

        console.log('avgValsLine', avgValsLine);
    }

    for (const lineIndex in lines) 
    {
        const line = lines[lineIndex];

        if (isBlank(line)) continue;

        const sl = line.split(';'); // splitted line
        const delimi = Math.ceil(lines.length / numberOfPoints); // delimitation
        const isLast = lineIndex == lines.length;

        if (lineIndex % delimi != delimi-1 && !isLast) 
        {
            if (averagingEnabled)
            {
                for (const vl of avgValsLine)
                {
                    for (const cellIndex in sl) 
                    {
                        const cell = sl[cellIndex];

                        for (const da of datasets)
                        {
                            if (shl[cellIndex] == vl.name)
                            {
                                vl.suma += parseInt(cell);
                                vl.count++;
                            }
                        }
                    }
                }
            }

            continue;
        }

        labels.push(sl[dateCol]);
            
        for (const cellIndex in sl) 
        {
            const cell = sl[cellIndex];

            for (const da of datasets)
            {
                if (shl[cellIndex] == da.label)
                {
                    if (averagingEnabled)
                    {
                        for (const vl of avgValsLine)
                        {
                            if (vl.name == da.label)
                            {
                                console.log(vl.suma, vl.count, (vl.suma / vl.count));
                                da.data.push(vl.suma / vl.count);
                                vl.suma = 0;
                                vl.count = 0;
                            }
                        }
                    }
                    else
                    {
                        da.data.push(cell);
                    }
                }
            }
        }
    }
    
    /*
    const data = {
        labels: labels,
        datasets: datasets,
    };
    */
    const data = {
        datasets: [
            {
                label: "Hovínko",
                data: [
                    {'x': "2021-09-26 22:24:19",'y': 2},
                    {'x': "2021-09-26 22:27:19",'y': 3},
                    {'x': "2021-09-26 22:28:19",'y': 6},
                ],
                fill: false,
                borderColor: 'red',
            }
        ],
    };
    
    console.log(data);

    const config = {
        type: 'line',
        data: data,
        options: {
            responsive: true,
            plugins: {
                title: {
                    display: true,
                    text: "Data Graph of ESP32",
                    color: "#ccc",
                    align: "top",
                    font: {
                        size: 24,
                    },
                },
            },
            scales: {
                
                xAxes: [{
                    type: 'time',
                    time: {
                        displayFormats: {
                        }
                    },
                    ticks: {
                        autoSkip: false,
                        maxTicksLimit: 20,
                    },
                }],
                
                yAxes: [{
                    ticks: {
                        autoSkip: true,
                        maxTicksLimit: 8,
                    },
                }]
            },
        },
    };

    var myChart = new Chart(ctx, config);
}

function randomColor()
{
    const r = Math.random() * 255;
    const g = Math.random() * 255;
    const b = Math.random() * 255;

    if (r + g + b < 127) return randomColor();

    return `rgb(${r},${g},${b})`;
}

function isBlank(str) {
    return (!str || /^\s*$/.test(str));
}

fetch("http://185.221.124.205/esp32/data.txt")
   .then( r => r.text() )
   .then( t => createGraph(t))

</script>