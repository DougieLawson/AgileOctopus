function getData()
{
	$.getJSON("cgi-bin/agile.tariff", function (data)
	{
		var periodStart = [];
		var price = [];
		var bgColour = [];
		var boColour = [];
		data.forEach(function(item) {
			bgColour.push(item['backgroundColor']);
			periodStart.push(item['periodStart']);
			price.push(item['price']);
			boColour.push(item['borderColor']);
		});
		displayCh(bgColour, periodStart, price, boColour);
	});
};

function displayCh(w, x, y, z)
{
	new Chart(document.getElementById("myChart"),
	{
		"type":"bar",
		"data":
		{
			"labels":x,
			"datasets":
			[
				{
					"label":"Pence / Unit",
					"data":y,
					"fill":true,
					"backgroundColor": w, 
					"borderColor": z, 
					"borderWidth":1
				}
			]
		},
		"options":
		{
			"scales":
			{
				"yAxes":
				[
					{
						"ticks":{ "beginAtZero":true }
					}
				]
			}
			
		}
	}
	);
}
