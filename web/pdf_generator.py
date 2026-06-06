"""Professional aSc-style PDF timetable generator using ReportLab.

Produces per-class/stream timetable pages with:
- Subject abbreviations in bold (e.g., KIS, MATH)
- Teacher initials in bottom-right corner (e.g., PI)
- Subject color backgrounds
- Assembly/break markers in cells
"""

from datetime import datetime

try:
    from reportlab.lib import colors
    from reportlab.lib.pagesizes import A3, landscape
    from reportlab.lib.units import mm
    from reportlab.pdfgen import canvas as rl_canvas
    HAS_REPORTLAB = True

    # Module-level colors (must be outside try to be visible on import)
    HEADER_COLOR = colors.HexColor('#1e3a5f')
    ACCENT_COLOR = colors.HexColor('#f0a500')
    BORDER_COLOR = colors.HexColor('#cbd5e1')
    EVEN_ROW = colors.HexColor('#f8fafc')
    BREAK_BG = colors.HexColor('#fef3c7')
    BREAK_TEXT = colors.HexColor('#92400e')
    UNSCHED_COLOR = colors.HexColor('#dc2626')
except ImportError:
    HAS_REPORTLAB = False
    colors = None




def _rgb(hex_color):
    h = hex_color.lstrip('#')
    r = int(h[0:2], 16) / 255.0
    g = int(h[2:4], 16) / 255.0
    b = int(h[4:6], 16) / 255.0
    return (r, g, b)


def _time_label(tstr):
    """Convert '07:30' to '7:30' for display."""
    h, m = tstr.split(':')
    return f'{int(h)}:{m}'


def generate_timetable_pdf(slot_data, days, periods, unscheduled,
                           output_path, title='School Timetable',
                           school_name='Alliance High School',
                           class_timelines=None):
    """Generate a professional per-class timetable PDF with abbreviations.

    Args:
        slot_data: List of slot dicts with subjectAbbr, teacherAbbr,
                   subjectColor, className, dayId, periodId, start, end
        days: List of {id, name}
        periods: List of {id, start, end} (used only as fallback)
        unscheduled: List of unscheduled lesson info
        output_path: Path to write PDF to
        title: Report title
        school_name: School name for header
        class_timelines: Per-class timeline info for break display
    """
    if not HAS_REPORTLAB:
        return False

    from reportlab.lib.pagesizes import landscape, A3
    from reportlab.lib.units import mm

    # Group slots by class
    by_class = {}
    for s in slot_data:
        cname = s.get('className', 'Unknown')
        by_class.setdefault(cname, {})
        by_class[cname][(s['dayId'], s['periodId'])] = s

    sorted_classes = sorted(by_class.keys())
    num_classes = len(sorted_classes)

    day_ids = [d['id'] for d in days]
    day_names = [d['name'] for d in days]
    num_days = len(days)

    page_w, page_h = landscape(A3)
    margin = 10 * mm
    usable_w = page_w - 2 * margin
    usable_h = page_h - 2 * margin

    c = rl_canvas.Canvas(output_path, pagesize=landscape(A3))
    c.setTitle(f'{school_name} - {title}')

    # Layout constants
    title_h = 20 * mm
    meta_h = 8 * mm
    entity_title_h = 10 * mm
    header_h = 14 * mm
    row_h = 13 * mm
    break_row_h = 8 * mm
    period_col_w = 20 * mm
    day_col_w = (usable_w - period_col_w) / max(num_days, 1)

    # Calculate entities per page
    table_h = num_days * max(row_h, break_row_h) + header_h
    total_per_page = max(
        1, int((usable_h - title_h - meta_h - 5 * mm) / (
            entity_title_h + table_h + 6 * mm))
    )

    idx = 0
    while idx < num_classes:
        page_classes = sorted_classes[idx:idx + total_per_page]
        idx += total_per_page

        y_top = page_h - margin

        # ── School name & title ──
        c.setFont('Helvetica-Bold', 16)
        c.setFillColor(HEADER_COLOR)
        c.drawString(margin, y_top - 6 * mm, school_name)
        c.setFont('Helvetica', 10)
        c.setFillColor(colors.HexColor('#475569'))
        c.drawString(margin, y_top - 11 * mm, title)
        # Accent line
        c.setStrokeColor(ACCENT_COLOR)
        c.setLineWidth(1.5)
        c.line(margin, y_top - 13 * mm, margin + usable_w, y_top - 13 * mm)
        # Date right-aligned
        c.setFont('Helvetica', 7)
        c.setFillColor(colors.HexColor('#94a3b8'))
        c.drawRightString(
            margin + usable_w, y_top - 6 * mm,
            datetime.now().strftime('%Y-%m-%d %H:%M'))
        y_top -= title_h + meta_h

        for ei, cname in enumerate(page_classes):
            slots = by_class[cname]

            # Find the class ID for this class name (first match)
            cid = None
            for s in slot_data:
                if s.get('className') == cname:
                    cid = s.get('classId')
                    break

            ty = y_top - 4 * mm

            # ── Class title ──
            c.setFont('Helvetica-Bold', 13)
            c.setFillColor(HEADER_COLOR)
            c.drawString(margin, ty, cname)
            ty -= entity_title_h

            # ── Table header row ──
            c.setFillColor(HEADER_COLOR)
            c.roundRect(margin, ty - header_h, period_col_w, header_h,
                        1.5, fill=1, stroke=0)
            c.setFillColor(colors.white)
            c.setFont('Helvetica-Bold', 7)
            c.drawCentredString(
                margin + period_col_w / 2, ty - header_h / 2 - 2, 'Time')

            for di, dn in enumerate(day_names):
                dx = margin + period_col_w + di * day_col_w
                c.setFillColor(HEADER_COLOR)
                c.roundRect(dx, ty - header_h, day_col_w, header_h,
                            1.5, fill=1, stroke=0)
                c.setFillColor(colors.white)
                c.setFont('Helvetica-Bold', 7)
                c.drawCentredString(
                    dx + day_col_w / 2, ty - header_h / 2 - 2,
                    dn[:3])  # Mon, Tue, Wed, Thu, Fri
            ty -= header_h

            # ── Build per-day period grid for this class ──
            # Incorporate breaks from class_timelines into the slot grid
            day_cells = {}  # day_id -> list of (slot_index, type, data)

            if class_timelines and cid and cid in class_timelines:
                tl = class_timelines[cid]
                for did, blocks in tl.get('blocks_by_day', {}).items():
                    day_cells[did] = []
                    for blk in blocks:
                        if blk['type'] == 'lesson':
                            slot_key = (did, blk['slot_index'])
                            s = slots.get(slot_key)
                            day_cells[did].append({
                                'type': 'lesson',
                                'slot_index': blk['slot_index'],
                                'start': blk['start_time'],
                                'end': blk['end_time'],
                                'label': blk['label'],
                                'slot': s,
                            })
                        else:
                            day_cells[did].append({
                                'type': 'break',
                                'label': blk['label'],
                                'start': blk['start_time'],
                                'end': blk['end_time'],
                                'slot_index': blk['slot_index'],
                                'slot': None,
                            })

            # Find the max number of blocks across all days for this class
            max_blocks = max(
                (len(cells) for cells in day_cells.values()), default=0
            )

            # ── Render rows ──
            for bi in range(max_blocks):
                # Collect info across days for this row
                row_items = {}
                max_row_h = row_h
                for di, did in enumerate(day_ids):
                    cells = day_cells.get(did, [])
                    if bi < len(cells):
                        row_items[did] = cells[bi]
                        if cells[bi]['type'] == 'break':
                            max_row_h = max(max_row_h, break_row_h)

                rh = max_row_h

                # Time column
                px = margin
                c.setFillColor(colors.HexColor('#f1f5f9'))
                c.rect(px, ty - rh, period_col_w, rh, fill=1, stroke=0)
                c.setStrokeColor(BORDER_COLOR)
                c.setLineWidth(0.4)
                c.rect(px, ty - rh, period_col_w, rh, fill=0, stroke=1)

                # Find the first item to show time in the period column
                first_item = next(iter(row_items.values()), None)
                if first_item:
                    time_str = f'{_time_label(first_item["start"])}-{_time_label(first_item["end"])}'
                else:
                    time_str = ''

                c.setFont('Helvetica-Bold', 6.5)
                c.setFillColor(colors.HexColor('#475569'))
                c.drawCentredString(
                    px + period_col_w / 2, ty - rh / 2 - 1.5, time_str)

                # Day columns
                for di, did in enumerate(day_ids):
                    dx = px + period_col_w + di * day_col_w
                    item = row_items.get(did)

                    if item and item['type'] == 'break':
                        # Break cell
                        c.setFillColor(BREAK_BG)
                        c.rect(dx, ty - rh, day_col_w, rh, fill=1, stroke=0)
                        c.setStrokeColor(BORDER_COLOR)
                        c.setLineWidth(0.4)
                        c.rect(dx, ty - rh, day_col_w, rh, fill=0, stroke=1)
                        c.setFont('Helvetica', 6)
                        c.setFillColor(BREAK_TEXT)
                        c.drawCentredString(
                            dx + day_col_w / 2, ty - rh / 2 - 2,
                            item['label'])
                    elif item and item['type'] == 'lesson' and item['slot']:
                        slot = item['slot']
                        col_hex = slot.get('subjectColor', '#4A90D9')
                        r, g, b = _rgb(col_hex)

                        # Tinted background
                        c.setFillColor(colors.Color(r, g, b, alpha=0.15))
                        c.rect(dx + 0.5, ty - rh + 0.5, day_col_w - 1,
                               rh - 1, fill=1, stroke=0)
                        c.setStrokeColor(BORDER_COLOR)
                        c.setLineWidth(0.4)
                        c.rect(dx, ty - rh, day_col_w, rh, fill=0, stroke=1)

                        # Subject abbreviation (bold, centered)
                        abbr = slot.get('subjectAbbr', slot.get(
                            'subjectName', '')[:3].upper())
                        c.setFont('Helvetica-Bold', 8)
                        c.setFillColor(colors.HexColor('#0f172a'))
                        c.drawCentredString(
                            dx + day_col_w / 2, ty - rh / 2 + 2, abbr)

                        # Teacher initials (bottom-right)
                        t_abbr = slot.get('teacherAbbr', '')
                        if t_abbr:
                            c.setFont('Helvetica', 5.5)
                            c.setFillColor(colors.HexColor('#64748b'))
                            c.drawRightString(
                                dx + day_col_w - 1.5, ty - rh + 1.5, t_abbr)

                        # Week badge
                        wt = slot.get('weekType', 0)
                        if wt == 1:
                            c.setFont('Helvetica', 5)
                            c.setFillColor(colors.HexColor('#2563eb'))
                            c.drawString(dx + 1, ty - rh + 1, 'A')
                        elif wt == 2:
                            c.setFont('Helvetica', 5)
                            c.setFillColor(colors.HexColor('#9333ea'))
                            c.drawString(dx + 1, ty - rh + 1, 'B')
                    else:
                        # Empty cell (no lesson and not break)
                        even = bi % 2 == 0
                        c.setFillColor(colors.white if even else EVEN_ROW)
                        c.rect(dx, ty - rh, day_col_w, rh, fill=1, stroke=0)
                        c.setStrokeColor(BORDER_COLOR)
                        c.setLineWidth(0.4)
                        c.rect(dx, ty - rh, day_col_w, rh, fill=0, stroke=1)

                ty -= rh

            # ── Spacing ──
            y_top = ty - 5 * mm
            if ei < len(page_classes) - 1:
                c.setStrokeColor(colors.HexColor('#e2e8f0'))
                c.setLineWidth(0.5)
                c.line(margin, y_top + 2 * mm, margin + usable_w,
                       y_top + 2 * mm)

        # ── Unscheduled ──
        if unscheduled:
            uy = y_top - 10 * mm
            c.setFont('Helvetica-Bold', 10)
            c.setFillColor(UNSCHED_COLOR)
            c.drawString(margin, uy,
                         f'Unscheduled Lessons ({len(unscheduled)})')
            uy -= 6 * mm

            col_widths = [55 * mm, 55 * mm, 40 * mm, 25 * mm, 60 * mm]
            headers = ['Class', 'Subject', 'Teacher', 'Periods', 'Reason']
            hx = margin
            c.setFont('Helvetica-Bold', 6)
            c.setFillColor(colors.white)
            for hw, hdr in zip(col_widths, headers):
                c.setFillColor(UNSCHED_COLOR)
                c.rect(hx, uy - 5 * mm, hw, 5 * mm, fill=1, stroke=0)
                c.setFillColor(colors.white)
                c.drawString(hx + 1 * mm, uy - 3.5 * mm, hdr)
                hx += hw
            uy -= 5 * mm

            for u in unscheduled[:20]:
                hx = margin
                c.setFont('Helvetica', 6)
                c.setFillColor(colors.HexColor('#1e293b'))
                vals = [
                    str(u.get('className', '')),
                    str(u.get('subjectName', '')),
                    str(u.get('teacherName', '')),
                    str(u.get('periodsCount', 0)),
                    str(u.get('reason', '')),
                ]
                for vw, val in zip(col_widths, vals):
                    c.drawString(hx + 1 * mm, uy - 3.5 * mm, val)
                    hx += vw
                uy -= 4.5 * mm

            if len(unscheduled) > 20:
                c.setFont('Helvetica-Oblique', 7)
                c.setFillColor(colors.HexColor('#94a3b8'))
                c.drawString(
                    margin, uy,
                    f'... and {len(unscheduled) - 20} more')

        c.showPage()

    c.save()
    return True
