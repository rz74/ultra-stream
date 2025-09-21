# evaluate_results.py
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import base64
from jinja2 import Template
from io import BytesIO

latency_csv = "test_results/latency_trace.csv"
report_html = "test_results/latency_report.html"

# Load and compute
print("[INFO] Loading data...")
lat = pd.read_csv(latency_csv)

# Force numeric conversion
cols = ["t_recv", "t_parsed", "t_calc_start", "t_calc_end", "t_sent", "num_updates"]
lat[cols] = lat[cols].apply(pd.to_numeric, errors="coerce")

print("[INFO] Computing latency stats...")
lat["net_us"] = (lat["t_sent"] - lat["t_recv"] - (lat["t_calc_end"] - lat["t_calc_start"])) / 1000
lat["calc_us"] = (lat["t_calc_end"] - lat["t_calc_start"]) / 1000
lat["total_us"] = (lat["t_sent"] - lat["t_recv"]) / 1000

# Summary statistics
summary = lat[["net_us", "calc_us", "total_us"]].describe(percentiles=[.5, .9, .99]).round(2)
grouped = lat.groupby("num_updates")["net_us"].describe(percentiles=[.5, .9, .99]).round(2)
grouped_html = grouped.to_html(classes="grouped", border=0)

# Generate a histogram for each group
print("[INFO] Creating histograms per num_updates group...")
histograms = []
for num, group in lat.groupby("num_updates"):
    plt.figure()
    plt.hist(group["total_us"], bins=40, color='steelblue', alpha=0.75)
    plt.title(f"Latency Distribution for num_updates = {num}")
    plt.xlabel("Latency (microseconds)")
    plt.ylabel("Count")
    plt.grid(True)
    plt.tight_layout()

    buf = BytesIO()
    plt.savefig(buf, format="png")
    plt.close()
    encoded = base64.b64encode(buf.getvalue()).decode("utf-8")
    histograms.append((num, encoded))

# HTML tables
summary_html = summary.to_html(classes="summary", border=0)
top5_html = lat.sort_values("total_us", ascending=False).head().to_html(index=False)

# latency table
detailed = lat[[
    "subject_id", "num_updates", "t_recv", "t_parsed", "t_calc_start", "t_calc_end", "t_sent",
    "net_us", "calc_us", "total_us"
]]
full_table_html = detailed.round(2).to_html(index=False)

# HTML report
template = Template("""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Latency Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 2em; }
        table { border-collapse: collapse; margin-bottom: 2em; font-size: 13px; }
        th, td { border: 1px solid #ccc; padding: 0.4em 0.8em; text-align: right; }
        th { background-color: #f0f0f0; }
        h1, h2 { color: #333; }
        .section { margin-top: 2em; }
    </style>
</head>
<body>
    <h1>Latency Report</h1>

    <div class="section">
        <h2>Latency Summary (microseconds)</h2>
        {{ summary_table | safe }}
    </div>

    <div class="section">
        <h2>Top 5 Slowest Events</h2>
        {{ slowest_table | safe }}
    </div>

    <div class="section">
        <h2>Network Latency by Number of Updates</h2>
        {{ grouped_table | safe }}
    </div>

    <div class="section">
        <h2>Latency Histogram by num_updates</h2>
        {% for num, image_data in histogram_list %}
            <h3>num_updates = {{ num }}</h3>
            <img src="data:image/png;base64,{{ image_data }}" width="600"/><br/>
        {% endfor %}
    </div>

    <div class="section">
        <h2>Detailed Latency Table</h2>
        {{ full_table | safe }}
    </div>
</body>
</html>
""")

html = template.render(
    summary_table=summary_html,
    slowest_table=top5_html,
    grouped_table=grouped_html,
    histogram_list=histograms,
    full_table=full_table_html
)

with open(report_html, "w") as f:
    f.write(html)

print(f"[INFO] Report written to {report_html}")
